//
//  DevicesTableViewController.swift
//  Arduino Control
//
//  Created by Guilherme Rambo on 25/04/16.
//  Copyright © 2016 Guilherme Rambo. All rights reserved.
//

import UIKit
import CoreBluetooth

let BLEServiceUUID = CBUUID(string: "FFE0")
let SerialCharacteristicUUID = CBUUID(string: "FFE1")

class DevicesTableViewController: UITableViewController, CBCentralManagerDelegate {

    enum Notifications: String {
        case BluetoothDeviceConnected
        case BluetoothDeviceFailedToConnect
    }
    
    private var centralManager: CBCentralManager?
    private var connectedDevice: CBPeripheral?
    
    private var devices = [CBPeripheral]()
    
    override func viewDidLoad() {
        super.viewDidLoad()

        let centralQueue = dispatch_queue_create("devicesQueue", DISPATCH_QUEUE_SERIAL)
        centralManager = CBCentralManager(delegate: self, queue: centralQueue)
        
        startSearch()
        
        NSNotificationCenter.defaultCenter().addObserver(self, selector: #selector(teardown), name: UIApplicationWillTerminateNotification, object: nil)
    }
    
    @objc private func teardown() {
        if let connectedDevice = connectedDevice {
            guard connectedDevice.state != .Disconnected else { return }
            
            centralManager?.cancelPeripheralConnection(connectedDevice)
        }
    }
    
    private func startSearch() {
        guard let manager = centralManager where !manager.isScanning else { return }
        
        manager.scanForPeripheralsWithServices([BLEServiceUUID, SerialCharacteristicUUID], options: nil)
        
        let connectedPeripherals = manager.retrieveConnectedPeripheralsWithServices([BLEServiceUUID, SerialCharacteristicUUID])
        connectedPeripherals.forEach { self.addDevice($0) }
    }
    
    private func stopScan() {
        guard let manager = centralManager where manager.isScanning else { return }
        
        manager.stopScan()
        UIApplication.sharedApplication().networkActivityIndicatorVisible = false
    }
    
    override func viewDidAppear(animated: Bool) {
        super.viewDidAppear(animated)
        
        startSearch()
    }
    
    override func viewWillDisappear(animated: Bool) {
        super.viewWillDisappear(animated)
        
        stopScan()
    }
    
    private func addDevice(device: CBPeripheral) {
        guard !devices.contains(device) else { return }
        
        devices.append(device)
        
        tableView.insertRowsAtIndexPaths([NSIndexPath(forRow: devices.count - 1, inSection: 0)], withRowAnimation: UITableViewRowAnimation.Automatic)
    }
    
    func centralManager(central: CBCentralManager, didDiscoverPeripheral peripheral: CBPeripheral, advertisementData: [String : AnyObject], RSSI: NSNumber) {
        NSLog("Did discover peripheral: \(peripheral). Data: \(advertisementData). RSSI: \(RSSI)")
        
        dispatch_async(dispatch_get_main_queue()) {
            self.addDevice(peripheral)
        }
    }
    
    func centralManagerDidUpdateState(central: CBCentralManager) {
        switch central.state {
        case .PoweredOff:
            UIApplication.sharedApplication().networkActivityIndicatorVisible = false
            NSLog("Central powered off")
        case .PoweredOn:
            UIApplication.sharedApplication().networkActivityIndicatorVisible = true
            NSLog("Central powered on")
        case .Resetting:
            NSLog("Central resetting")
        case .Unauthorized:
            NSLog("Central unauthorized")
        default:
            NSLog("Unknown central state")
        }
    }
    
    private func connectToDevice(device: CBPeripheral) {
        centralManager?.connectPeripheral(device, options: nil)
    }
    
    func centralManager(central: CBCentralManager, didConnectPeripheral peripheral: CBPeripheral) {
        NSNotificationCenter.defaultCenter().postNotificationName(Notifications.BluetoothDeviceConnected.rawValue, object: self)
    }
    
    func centralManager(central: CBCentralManager, didFailToConnectPeripheral peripheral: CBPeripheral, error: NSError?) {
        var userInfo = [NSObject:AnyObject]()
        if let error = error {
            userInfo["error"] = error
        }
        
        NSNotificationCenter.defaultCenter().postNotificationName(Notifications.BluetoothDeviceFailedToConnect.rawValue, object: self, userInfo: userInfo)
    }

    // MARK: - Table view data source

    override func numberOfSectionsInTableView(tableView: UITableView) -> Int {
        return 1
    }

    override func tableView(tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return devices.count
    }

    
    override func tableView(tableView: UITableView, cellForRowAtIndexPath indexPath: NSIndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCellWithIdentifier("device", forIndexPath: indexPath)

        if let name = devices[indexPath.row].name {
            cell.textLabel?.text = name
        } else {
            cell.textLabel?.text = devices[indexPath.row].identifier.UUIDString
        }
        
        return cell
    }
    
    // MARK: - Navigation
    
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject?) {
        if segue.identifier == "connect" {
            guard let selectedPath = tableView.indexPathForSelectedRow else { return }
            guard let destination = segue.destinationViewController as? RemoteDeviceViewController else { return }
            
            let device = devices[selectedPath.row]
            
            destination.device = device
            connectToDevice(device)
        }
    }

}
