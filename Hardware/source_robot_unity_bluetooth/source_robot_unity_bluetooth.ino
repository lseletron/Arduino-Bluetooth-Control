
/*
    Controle de Unidade Robótica com Aplicativo iOS Através de Módulo Bluetooth
    
    
    Autores
    
           Guilherme Rambo
           Wagner Rambo
           
    Data: Maio de 2016

*/


// --- Bibliotecas Auxiliares ---
#include <SoftwareSerial.h>  //Inclui biblioteca SoftwareSerial
#include <AFMotor.h>         //Inclui biblioteca AF Motor

// pinos para conexão com o módulo bluetooth via software serial
// RX (14 ou A0) ARDUINO <- TX Módulo
// TX (15 ou A1) ARDUINO -> RX Módulo
SoftwareSerial btModule(14, 15); // RX, TX


// --- Seleção dos Motores ---
AF_DCMotor motor1(1); //Seleção do Motor 1
AF_DCMotor motor2(2); //Seleção do Motor 2
AF_DCMotor motor3(3); //Seleção do Motor 3
AF_DCMotor motor4(4); //Seleção do Motor 4


// --- Mapeamento de Hardware ---
#define   buzzer   16   //Controle do buzzer pelo pino A2 (será utilizado como digital)


// --- Protótipo das Funções Auxiliares ---
void parseCommand(char *command);       //Função para interpretar os comandos via BlueTooth
void robot_forward(unsigned char v);    //Função para movimentar robô para frente
void robot_backward(unsigned char v);   //Função para movimentar robô para trás
void robot_left(unsigned char v);       //Função para movimentar robô para esquerda
void robot_right(unsigned char v);      //Função para movimentar robô para direita
void robot_stop(unsigned char v);       //Função para parar o robô
void watchdog();                        //Função para parar o robô caso a conexão bluetooth com o iOS seja perdida



// --- Variáveis Globais ---

const int BUFFERSIZE = 32;             // tamanho do buffer de comando
char *commandBuffer;                   // buffer para armazenar caracteres recebidos via bluetooth
char *curCommand;                      // ponteiro para a string do comando atual
char bufPos = 0;                       // posição do caractere atual no buffer de comando
const char separator = '\n';           // separador de comandos (quando este caractere é recebido,
                                       // o comando atual no buffer é interpretado e executado)
                                       
// ponteiro para a última função de movimento executada, utilizado quando a velocidade é alterada
void (*last_movement)(unsigned char v) = NULL;

// -- Flags Auxiliares --
boolean flag_up, flag_down, flag_right, flag_left, flag_horn, flag_break;
unsigned char velocidade = 0x00;       // armazena a velocidade dos motores (8 bits)
unsigned long lastPingTime = 0; // tempo de recebimento do último "PING"
const unsigned long pingTimeout = 2000; // tempo máximo entre "PINGs" permitido antes do robô parar por segurança
         

// --- Configurações Iniciais ---
void setup()
{

  commandBuffer = (char *)malloc(BUFFERSIZE);        //Aloca memória para o buffer de comando
  
  pinMode(buzzer, OUTPUT);                           //Pino buzzer configurado como saída digital  
  pinMode(13,OUTPUT);                                //Pino do LED onboard configurado como saída digital
  
  digitalWrite(buzzer, LOW);                         //Inicializa buzzer
  digitalWrite(13, LOW);                             //Inicializa LED
  
  //Descomente se for usar a Serial para debug do sistema
  //Serial.begin(9600);
  
  velocidade = 0xFF; //Inicia velocidade no valor máximo
  
  
  btModule.begin(9600);                              //Comunicação em 9600 baud rate
  
} //end setup


// --- Loop Infinito ---
void loop()
{
  // verifica se a conexão está OK
  watchdog();
  
  if (btModule.available())                //btModule?
  {                                        //Ok...
    
          delay(10);                       //Tempo de espera sugerido
      
          char curChar = btModule.read(); // armazena o caractere recebido
          
      //    Descomente a linha abaixo para enviar comandos recebidos no módulo bluetooth para o monitor serial
          Serial.write(curChar);
       
          
          if (curChar == separator)       // separator \n encontrado?
          {                               //Sim...
          
            // cria um novo ponteiro para armazenar o comando atual
            curCommand = (char*)malloc(BUFFERSIZE - bufPos);
            memset(curCommand, 0, bufPos);
            
            // copia o comando do buffer para o novo ponteiro
            memcpy(curCommand, commandBuffer, BUFFERSIZE - bufPos);
            
            // chama função que irá interpretar o comando
            parseCommand(curCommand);
            
            // reseta o buffer
            bufPos = 0;
            memset(commandBuffer, 0, BUFFERSIZE);
          } 
          
          else 
          {
            // escreve o caractere recebido no buffer, na posição atual
            commandBuffer[bufPos] = curChar;
            // incrementa posição no buffer
            bufPos++;
            
          } //end else
    
  } //end if btModule.available
  
  
// descomente linhas abaixo para enviar comandos recebidos do monitor serial para o módulo bluetooth
  if (Serial.available()) 
  {
    delay(10);
    btModule.write(Serial.read());
    
  } //end if Serial.available
  
  
//==================================================================================// 
// TESTES PARA VERIFICAR O COMANDO RECEBIDO E EXECUTAR A RESPECTIVA AÇÃO DO ROBÔ

  if(flag_up)              //Botão UP pressionado?
  {                        //Sim...
     flag_up = 0x00;       //Limpa flag
     
     //Move Robô para frente
     robot_forward(velocidade);
     
     //Grava ponteiro para último movimento, caso velocidade mude
     last_movement = robot_forward;
  
  } //end UP
  
  if(flag_down)            //Botão DOWN pressionado?
  {                        //Sim...
     flag_down = 0x00;     //Limpa flag
     
     //Move Robô para trás
     robot_backward(velocidade);
     
     //Grava ponteiro para último movimento, caso velocidade mude
     last_movement = robot_backward;
  
  } //end DOWN
  
   if(flag_right)          //Botão RIGHT pressionado?
  {                        //Sim...
     flag_right = 0x00;    //Limpa flag
     
     //Move Robô para direita
     robot_right(velocidade);

     
     //Grava ponteiro para último movimento, caso velocidade mude
     last_movement = robot_right;
  
  } //end RIGHT
  
  if(flag_left)            //Botão LEFT pressionado?
  {                        //Sim...
     flag_left = 0x00;     //Limpa flag
     
     //Move Robô para esquerda
     robot_left(velocidade);

     
     //Grava ponteiro para último movimento, caso velocidade mude
     last_movement = robot_left;
  
  } //end LEFT
  
  
  if(flag_break)           //Botão BREAK pressionado?
  {                        //Sim...
     flag_break = 0x00;    //Limpa flag
     
     //Para o Robô
     robot_stop(velocidade);
     
     //Limpa ponteiro para último movimento
     last_movement = NULL;
  } //end BREAK
  
  
  if(flag_horn)           //Botão HORN pressionado?
  {                       //Sim...
     flag_horn = 0x00;    //Limpa flag
     
     //Limpa ponteiro para último movimento
     last_movement = NULL;
     
     //Soa buzzer
     digitalWrite(buzzer, HIGH);
     delay(800);
     digitalWrite(buzzer, LOW);
  } //end HORN
  
   
  
   
} //end loop


// --- Desenvolvimento das Funções Auxiliares ---

// esta função verifica se o útlimo "PING" do app foi recebido há mais de 2 segundos e caso
// positivo, para o robô pois isso significa que perdemos a conexão com o controle remoto
void watchdog()
{
  // ignora se lastPingTime ainda não foi definido
  if (lastPingTime == 0) return;

  // tempo atual
  unsigned long curTime = millis();

  // verifica se o último ping foi recebido há mais de 2 segundos
  if (curTime - lastPingTime >= pingTimeout) {
    // caso sim, para o robô, por segurança
    robot_stop(velocidade);
  }
}

// esta função é o "cérebro" que irá interpretar os comandos recebidos via bluetooth e executar as ações necessárias
void parseCommand(char *command)
{
  Serial.write(command);

  if (strcmp(command,"PING") == 0) {
    // recebeu "PING", guarda o tempo em que foi recebido (usado na função watchdog)
    lastPingTime = millis();
  }

  // analisa cada comando válido, setando a respectiva flag
  flag_up    = (strcmp(command,"UP")    == 0);
  flag_down  = (strcmp(command,"DOWN")  == 0);
  flag_right = (strcmp(command,"RIGHT") == 0);
  flag_left  = (strcmp(command,"LEFT")  == 0);
  flag_horn  = (strcmp(command,"HORN")  == 0);
  flag_break = (strcmp(command,"BREAK") == 0);

  // -- comando de velocidade --
  
  // transforma command num objeto String
  String cmdStr = String(command);

  // verifica se a string começa com "V=" (comando de velocidade)
  if (cmdStr.indexOf("V=") == 0) {
    // pega os últimos 3 caracteres, que são o valor da velocidade ex: "255"
    String cmdV = cmdStr.substring(2, 5);
    // converte em int e armazena
    velocidade = cmdV.toInt();

    // executa movimento novamente com nova velocidade
    if (last_movement != NULL) {
      last_movement(velocidade);
    }

    // como o comando de velocidade é enviado rapidamente, precisamos resetar o ping também,
    // caso contrário o PING do app pode não ser recebido a tempo e o robô irá parar
    lastPingTime = millis();
  }
  
  // -- fim do comando de velocidade --

  // pisca o LED interno do Arduino para indicar que um comando foi recebido
  digitalWrite(13, HIGH);
  delay(90);
  digitalWrite(13, LOW);

  // libera a memória usada pelo ponteiro
  free(command);
  
} //end parseCommand


void robot_forward(unsigned char v)
{
     motor1.setSpeed(v);
     motor1.run(FORWARD);
     motor2.setSpeed(v);
     motor2.run(FORWARD);
     motor3.setSpeed(v);
     motor3.run(FORWARD);
     motor4.setSpeed(v);
     motor4.run(FORWARD);

} //end robot forward


void robot_backward(unsigned char v)
{
     motor1.setSpeed(v);
     motor1.run(BACKWARD);
     motor2.setSpeed(v);
     motor2.run(BACKWARD);
     motor3.setSpeed(v);
     motor3.run(BACKWARD);
     motor4.setSpeed(v);
     motor4.run(BACKWARD);

} //end robot backward


void robot_left(unsigned char v)
{
     motor1.setSpeed(v);
     motor1.run(FORWARD);
     motor2.setSpeed(v);
     motor2.run(FORWARD);
     motor3.setSpeed(v);
     motor3.run(BACKWARD);
     motor4.setSpeed(v);
     motor4.run(BACKWARD);

} //end robot left


void robot_right(unsigned char v)
{
     motor1.setSpeed(v);
     motor1.run(BACKWARD);
     motor2.setSpeed(v);
     motor2.run(BACKWARD);
     motor3.setSpeed(v);
     motor3.run(FORWARD);
     motor4.setSpeed(v);
     motor4.run(FORWARD);

} //end robot right


void robot_stop(unsigned char v)
{
     motor1.setSpeed(v);
     motor1.run(RELEASE);
     motor2.setSpeed(v);
     motor2.run(RELEASE);
     motor3.setSpeed(v);
     motor3.run(RELEASE);
     motor4.setSpeed(v);
     motor4.run(RELEASE);

} //end robot stop

