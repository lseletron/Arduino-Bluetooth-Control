//
//  RemoteDeviceViewController.swift
//  Arduino Control
//
//  Created by Guilherme Rambo on 25/04/16.
//  Copyright © 2016 Guilherme Rambo. All rights reserved.
//

import UIKit
import CoreBluetooth

class RemoteDeviceViewController: UIViewController, CBPeripheralDelegate {

    @IBOutlet weak private var controlsContainerView: UIView!
    @IBOutlet weak private var connectionStatusContainerView: UIView!
    @IBOutlet weak private var connectionStatusLabel: UILabel!
    @IBOutlet weak private var connectionStatusActivityIndicator: UIActivityIndicatorView!
    
    var pingTimer: NSTimer!
    
    var device: CBPeripheral? {
        didSet {
            updateUI()
        }
    }
    var characteristic: CBCharacteristic!
    
    private func showConnectionStatus(message: String = "Conectando") {
        dispatch_async(dispatch_get_main_queue()) {
            self.connectionStatusLabel.text = message
            self.connectionStatusActivityIndicator.startAnimating()
            self.connectionStatusContainerView.hidden = false
        }
    }
    
    private func hideConnectionStatus() {
        dispatch_async(dispatch_get_main_queue()) {
            self.connectionStatusActivityIndicator.stopAnimating()
            self.connectionStatusContainerView.hidden = true
            
            self.controlsContainerView.hidden = false
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

        showConnectionStatus()
        
        NSNotificationCenter.defaultCenter().addObserver(self, selector: #selector(didConnectDevice(_:)), name: DevicesTableViewController.Notifications.BluetoothDeviceConnected.rawValue, object: nil)
        NSNotificationCenter.defaultCenter().addObserver(self, selector: #selector(failedToConnectDevice(_:)), name: DevicesTableViewController.Notifications.BluetoothDeviceFailedToConnect.rawValue, object: nil)
    
        updateUI()
    }
    
    @objc private func didConnectDevice(note: NSNotification) {
        NSLog("Bluetooth connection succeeded")
        
        guard let device = device else { return }
        
        NSLog("Looking for services")
        
        device.delegate = self
        device.discoverServices([BLEServiceUUID])
    }
    
    func peripheral(peripheral: CBPeripheral, didDiscoverServices error: NSError?) {
        guard error == nil else {
            NSLog("Failed to discover services: \(error)")
            showConnectionStatus("Conexão falhou")
            return
        }
        
        guard let services = peripheral.services where services.count > 0 else {
            showConnectionStatus("Sem serviços")
            return
        }
        
        NSLog("Looking for characteristics")
        
        services.forEach { peripheral.discoverCharacteristics([SerialCharacteristicUUID], forService: $0) }
    }
    
    func peripheral(peripheral: CBPeripheral, didDiscoverCharacteristicsForService service: CBService, error: NSError?) {
        guard let characteristics = service.characteristics where characteristics.count > 0 else {
            NSLog("Service \(service) has no characteristics")
            showConnectionStatus("Sem características")
            return
        }
        
        NSLog("Characteristics found:")
        guard let serialCharacteristic = characteristics.filter({ $0.UUID == SerialCharacteristicUUID }).last else {
            NSLog("Position char not found")
            showConnectionStatus("Característica inválida")
            return
        }
        
        characteristic = serialCharacteristic
        
        NSLog("Found position char");
        peripheral.setNotifyValue(true, forCharacteristic: characteristic);
        
        hideConnectionStatus()
        sendCommand("CONNECTED\n")
        dispatch_async(dispatch_get_main_queue()) {
            self.startPing()
        }
        
        peripheral.readValueForCharacteristic(characteristic)
    }
    
    func peripheral(peripheral: CBPeripheral, didUpdateValueForDescriptor descriptor: CBDescriptor, error: NSError?) {
        NSLog("didUpdateValueForDescriptor \(descriptor)")
    }
    
    func sendCommand(command: String) {
        guard characteristic != nil else { return }
        guard let dataToWrite = command.dataUsingEncoding(NSASCIIStringEncoding) else { return }
        
        device?.writeValue(dataToWrite, forCharacteristic: characteristic, type: .WithResponse)
        NSLog("\(command)")
    }
    
    @objc private func failedToConnectDevice(note: NSNotification) {
        NSLog("Bluetooth connection failed \(note.userInfo)")
        
        showConnectionStatus("Conexão falhou")
    }

    private func updateUI() {
        guard let device = device else { return }
        
        title = device.name ?? device.identifier.UUIDString
    }
    
    @IBAction func sendUpCommand(sender: AnyObject) {
        sendCommand("UP\n")
    }
    @IBAction func sendRightCommand(sender: AnyObject) {
        sendCommand("RIGHT\n")
    }
    @IBAction func sendDownCommand(sender: AnyObject) {
        sendCommand("DOWN\n")
    }
    @IBAction func sendLeftCommand(sender: AnyObject) {
        sendCommand("LEFT\n")
    }
    @IBAction func sendBreakCommand(sender: AnyObject) {
        sendCommand("BREAK\n")
    }
    @IBAction func sendHornCommand(sender: AnyObject) {
        sendCommand("HORN\n")
    }
    @IBAction func sendVelocityCommand(sender: UISlider) {
        let velocity = String(format: "%03d", Int(sender.value))
        let command = "V=\(velocity)\n"
        sendCommand(command)
    }
    
    func startPing() {
        pingTimer = NSTimer.scheduledTimerWithTimeInterval(1.5, target: self, selector: #selector(sendPingCommand), userInfo: nil, repeats: true)
    }
    
    func sendPingCommand() {
        sendCommand("PING\n")
    }

}
