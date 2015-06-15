#include <SPI.h>
#include "si446x_api.h"

RadioSi446x si4463(0);
RadioSi446x si4463_1(1);


void setup() {
  // put your setup code here, to run once:
  // set the slaveSelectPin as an output:
//  pinMode (slaveSelectPin, OUTPUT);
//  //initialize SPI
//  SPI.begin();

//  printf("test!");
  Serial.begin(9600);
  si4463.setup();

  digitalWrite(RADIO_SDN_PIN, HIGH);  // active high shutdown = reset
  delay(600);
  digitalWrite(RADIO_SDN_PIN, LOW);   // booting
  delay(5);//delay 5ms for SDN pulled low
  Serial.println("Radio is powered up"); 

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(" test ");
  si4463.ptt_on();
  si4463_1.ptt_on();
  
  si4463.si4463_init();
  si4463_1.si4463_init();

  Serial.print("TX FIFO available length before write:");
  
  si4463.fifo_reset();
  si4463_1.fifo_reset();
/////////test, si4463 as reciver, si4463_1 as sender		
  si4463.enable_rx_interrupt();	
  si4463_1.enable_tx_interrupt();
  si4463.clr_interrupt();  // clr INT Factor
  si4463_1.clr_interrupt();	
  
  si4463_1.spi_write_fifo();//write fifo
  
//  Serial.print("Before change, State:");
//  si4463.request_device_state();
//  si4463_1.request_device_state();
  
  si4463.rx_start();
  si4463_1.tx_start();
  delayMicroseconds(200);
//  Serial.print("After change, State:");
//  si4463.request_device_state();
//  si4463_1.request_device_state();

  
  int jj = 3;
  Serial.println("========================");
  while(jj--){
    sleep(1);
    
    Serial.print("Check NIRQ:");
    Serial.println(digitalRead(NIRQ)); 
    if(!digitalRead(NIRQ)){ 
      Serial.println("Recv!");
//      si4463.clr_interrupt();   // clear interrupt
      ///////////////////////////////////////////
      Serial.print("Reciver: ");
      si4463.get_fifo_info();
      si4463.spi_read_fifo();
      si4463.get_fifo_info();
      //Serial.print("Reciver:After reset fifo: ");
      //si4463.fifo_reset();
      //si4463.get_fifo_info();
      si4463.clr_interrupt();   // clear interrupt
      ///////////////////////////////////////////

      //si4463.enable_rx_interrupt();	
      //si4463.clr_interrupt();  // clr INT Factor
     // si4463.rx_start();
/*      Serial.println(digitalRead(NIRQ));
      
      si4463.clr_interrupt();   // clear interrupt	
      Serial.print("FIFO:");
      
      si4463.spi_read_fifo();
      si4463.get_pakcet_info();
      si4463.fifo_reset();
      
      Serial.print("After reset FIFO:");
      si4463.clr_interrupt();   // clear interrupt	
      si4463.spi_read_fifo();*/
    }
    //si4463_1.fifo_reset();
    si4463_1.spi_write_fifo();//write fifo
          ///////////////////////////////////////////
      Serial.print("Sender: After Writing");
      si4463_1.get_fifo_info();

    si4463_1.tx_start();
    
          Serial.print("After send: ");
      si4463_1.get_fifo_info();
            Serial.print("Sender:After reset fifo: ");
      si4463_1.fifo_reset();
      si4463_1.get_fifo_info();
      ///////////////////////////////////////////
      Serial.println("========================");
  
  }
  
  si4463.spi_read_fifo();
  
  digitalWrite(RADIO_SDN_PIN, HIGH);
  exit(0);
}
