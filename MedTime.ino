/*
 * Copyright Danianny Gomes Dos Santos
 * 
 *     This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Tradução não-oficial:

    Este programa é um software livre; você pode redistribuí-lo e/ou 
    modificá-lo dentro dos termos da Licença Pública Geral GNU como 
    publicada pela Fundação do Software Livre (FSF); na versão 3 da 
    Licença, ou (na sua opinião) qualquer versão.

    Este programa é distribuído na esperança de que possa ser útil, 
    mas SEM NENHUMA GARANTIA; sem uma garantia implícita de ADEQUAÇÃO
    a qualquer MERCADO ou APLICAÇÃO EM PARTICULAR. Veja a
    Licença Pública Geral GNU para maiores detalhes.

    Você deve ter recebido uma cópia da Licença Pública Geral GNU junto
    com este programa. Se não, veja <http://www.gnu.org/licenses/>.

Para programas com mais de uma arquivo, é melhor substituir o “este pelo nome do programa, e começar a declaração com “Este arquivo é parte do programa 'NOME'”. Por exemplo

    This file is part of Foobar.

    Foobar is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

Tradução não-oficial:

    Este arquivo é parte do programa Foobar

    Foobar é um software livre; você pode redistribuí-lo e/ou 
    modificá-lo dentro dos termos da Licença Pública Geral GNU como 
    publicada pela Fundação do Software Livre (FSF); na versão 3 da 
    Licença, ou (na sua opinião) qualquer versão.

    Este programa é distribuído na esperança de que possa ser  útil, 
    mas SEM NENHUMA GARANTIA; sem uma garantia implícita de ADEQUAÇÃO
    a qualquer MERCADO ou APLICAÇÃO EM PARTICULAR. Veja a
    Licença Pública Geral GNU para maiores detalhes.

    Você deve ter recebido uma cópia da Licença Pública Geral GNU junto
    com este programa, Se não, veja <http://www.gnu.org/licenses/>.
 */

#include <EtherCard.h> //lib for enc28j60
#include <DS1307.h> //lib for rtc

#define SERIAL_BAUD 115200 //velocidade serial
#define ETH_MAC 0x74,0x69,0x69,0x2D,0x30,0x85 //endereco MAC
#define ETH_BSIZE 600 //tamanho do buffer
#define NUMBER_OF_LEDS 5 //quantidade de leds

static byte ethmac[] = { 
  ETH_MAC };
byte Ethernet::buffer[ETH_BSIZE];
static byte ip[4], gw[4];

int LED[NUMBER_OF_LEDS] = {
  7,6,5,4,3}; //pinagem
const int BUZZER PROGMEM = 2; //buzzer pino
const int FREQUENCIA PROGMEM = 100; //frequencia de som do buzzer
const int buttonPin PROGMEM= 9;     // pushbutton pino
byte buttonState = 0;         // pushbutton status ligado/desligado

typedef unsigned long int Timestamp;
Timestamp converterDataHora( String dt, String hr);

struct remedio
{
  byte caixa;
  byte intervalo;
  byte duracao;
  String data; //no formato dd.mm.yyyy
  String hora; // no formato hh:mm:ss
  Timestamp horaDoRemedio;
  int dose;
  byte ativado;
};

//para chamar posicao usar nome.atributo
struct remedio remedios[5];

//iniciar D21307
DS1307 rtc(A4, A5); //pinos

String hora;
String dataAtual; //hora e data atual
Timestamp atualTS, remedioTS;

//define conteudo da pagina html
const char html1[] PROGMEM =
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html\r\n"
"Content-length: 480\r\n"
"\r\n"
"<html><head><title>TCC</title></head><body>"
"<form method='GET'>Numero da caixa: <input type='number' name='caixa' min='1' max='5'><br>"
"Intervalo de horas: <input type='number' name='intervalo' min='1'><br>"
"Duracao (dias): <input type='number' name='duracaoDias' min='1'><br>"
"Inicio:<br>"
"Data (dd/mm/yyyy): <input type='text' name='data'><br>"
"Horario (hh:mm:ss): <input type='text' name='horario'><br>"
"<input type='submit'></form>"
"</body></html>"
;

int state;
char msg_status[40];

void send_page(const char *page, unsigned int pagelen);

void setup () {
  //seta relogio para run-mode
  rtc.halt(false);

  pinMode(BUZZER, OUTPUT); //inicializa o buzzer no pino
  digitalWrite(BUZZER, LOW); //buzzer comeca desligado
  pinMode(buttonPin, INPUT); //inicializa botao  

  for(byte i = 0; i < NUMBER_OF_LEDS; i++)
  { 
    pinMode(LED[i], OUTPUT); // inicializa o pino como uma saida
    //  digitalWrite(LED[i], LOW); //comecam desligados

    //setar valores da struct remedios  
    remedios[i].caixa = i;
    remedios[i].intervalo = 8; 
    remedios[i].duracao = 3;
    remedios[i].data = "08.11.2016";
    remedios[i].hora = "10:15:12";
    remedios[i].horaDoRemedio = 0;
    remedios[i].dose = 0;
    remedios[i].ativado = 1;
  }


  Serial.begin(SERIAL_BAUD); //inicia serial

  if (ether.begin(sizeof Ethernet::buffer, ethmac,8) == 0) {
    Serial.println(F("Failed to access Ethernet controller"));
    Serial.println(F("Aborted."));
    for(;;);
  }

  char *pip = "192.168.1.200"; //configura ip
  char *pgw = "192.168.1.1"; //configura gateway

  //configurar ip estatico
  Serial.print(F("IP:      ")); 
  Serial.println(pip);
  Serial.print(F("Gateway: ")); 
  Serial.println(pgw);
  ether.parseIp(ip, pip);
  ether.parseIp(gw, pgw);
  ether.staticSetup(ip, gw);
}

void loop () {
  word len = ether.packetReceive(); // go receive new packets
  word pos = ether.packetLoop(len); // respond to incoming pings
  char *buff = (char*)ether.tcpOffset();
  char *querystr;
  char *key, *value;
  char *data, *horario;
  int i;
  byte caixa, intervalo, duracaoDias;
  size_t slen;

  caixa       = -1;
  intervalo   = 0;
  duracaoDias = 0;
  horario     = NULL;
  data        = NULL;

  //  char *caixa, *intervalo, *dias;
  /* Check HTTP requests */
  if (pos > 0) {
    if (strstr(buff, "GET /") != 0) {
      if ((querystr = strstr(buff, "?caixa")) != 0) {
        //formato dos dados:
        //a=3&intervalo=3&duracaoDias=3&data=30%2F10%2F2016&horario=14%3A38%3A10 HTTP/1.1
        querystr++;

        key  = querystr;
        i    = 0;
        slen = strlen(querystr);
        while(i < slen) {
          if (querystr[i] == '=') {
            querystr[i] = '\0';
            i++;
            value = &querystr[i];
            while (i < slen && querystr[i] != '&' && querystr[i] != ' ') {
              if (i >= slen) {
                i--;
              } 
              else {
                i++;
              }
            }
            querystr[i] = '\0';
            i++;

            if (strncmp(key, "caixa", 5) == 0) {
              caixa = atoi(value);
              caixa --;
            } 
            else if (strncmp(key, "intervalo", 9) == 0) {
              intervalo = atoi(value);
            } 
            else if (strncmp(key, "duracaoDias", 11) == 0) {
              duracaoDias = atoi(value);
            } 
            else if (strncmp(key, "data", 4) == 0) {
              data = value;
            } 
            else if (strncmp(key, "horario", 7) == 0) {
              horario = value;
            }

            key = &querystr[i];
          } 
          else {
            i++;
          }
        }

        if (caixa >= 0 && caixa < NUMBER_OF_LEDS) {
          char tmp[11];
          remedios[caixa].intervalo = intervalo;
          remedios[caixa].duracao   = duracaoDias;

          tmp[0]  = data[0];
          tmp[1]  = data[1];
          tmp[2]  = '.';                                
          tmp[3]  = data[5];
          tmp[4]  = data[6];
          tmp[5]  = '.';                            
          tmp[6]  = data[10];
          tmp[7]  = data[11];
          tmp[8]  = data[12];
          tmp[9]  = data[13];
          tmp[10] = '\0'; 
          remedios[caixa].data = "";
          remedios[caixa].data.concat(tmp);
          // Serial.println(remedios[caixa].data);

          tmp[0] = horario[0];
          tmp[1] = horario[1];
          tmp[2] = ':';                                
          tmp[3] = horario[5];
          tmp[4] = horario[6];
          tmp[5] = ':';                            
          tmp[6] = horario[10];
          tmp[7] = horario[11];
          tmp[8] = '\0';
          remedios[caixa].hora = "";
          remedios[caixa].hora.concat(tmp);
          //  Serial.println(remedios[caixa].hora);
          remedios[caixa].horaDoRemedio = converterDataHora( remedios[caixa].data, remedios[caixa].hora);
          remedios[caixa].dose = ((remedios[caixa].duracao * 24)/remedios[caixa].intervalo);
          Serial.println(remedios[caixa].dose);
          remedios[caixa].ativado = 1; 
        }
      }
      send_page(html1, strlen_P(html1));
    }
  }

  dataAtual = rtc.getDateStr(); //setar data atual
  hora = rtc.getTimeStr(); //setar hora atual
  atualTS = converterDataHora( dataAtual, hora);
  remedioTS = atualTS + (5*60);
  Serial.println(atualTS);
  for(byte i = 0; i < NUMBER_OF_LEDS; i++)
  {
    //   Serial.println(dataAtual);
    //checar se esta no prazo p led acender
    if((remedios[i].horaDoRemedio >= atualTS) && (remedios[i].horaDoRemedio <= remedioTS) && (remedios[i].ativado == 1))
    {
      //acende luz
      digitalWrite(LED[remedios[i].caixa], HIGH);

      //toca buzzer
      digitalWrite(BUZZER, HIGH);
      delay(FREQUENCIA);
      digitalWrite(BUZZER, LOW);
      delay(FREQUENCIA);

      buttonState = digitalRead(buttonPin); //le o estado do botao

      if (buttonState == HIGH) 
      {  //checa se botao foi pressionado 
        remedios[i].dose --;
        // Serial.println(remedios[i].dose);
        remedios[i].ativado = 0;
        if(remedios[i].dose > 0){
          atualTS = converterDataHora( dataAtual, hora);
          remedios[i].horaDoRemedio = atualTS + (remedios[i].intervalo * 3600);
          remedios[i].ativado = 1;
        }
        // Serial.println(remedios[i].horaDoRemedio);
      } 
    }
    else 
    {
      digitalWrite(LED[remedios[i].caixa], LOW); //desliga led
      digitalWrite(BUZZER, LOW); //desliga buzzer
    }
  }
  //esperar um pouco para repetir
  delay(1000);
}

void send_page(const char *page, unsigned int pagelen)
{
  char *tcpbuff = (char*)ether.tcpOffset();

  strcpy_P(tcpbuff, page);

  ether.httpServerReply(pagelen);
}

Timestamp converterDataHora( String dt, String hr)
{
  //hr = hh:mm:ss
  //dt = dd/mm/yyyy
  unsigned int second = hr.substring(6,8).toInt();  // 0-59
  unsigned int minute = hr.substring(3,5).toInt();  // 0-59
  unsigned int horas   = hr.substring(0,2).toInt();    // 0-23
  unsigned int day    = dt.substring(0,2).toInt();   // 0-30
  unsigned int month  = (dt.substring(3,5).toInt() - 1); // 0-11
  unsigned int ano   = dt.substring(6,10).toInt();    // 0-99

  int mth[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334  };
  int mthb[12] = {
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335  };
  Timestamp timestamp =
    ( ( (!(ano % 4)) && (ano % 100) ) || (!(ano % 400)) )?
  ((((unsigned long int)( ano - 1970) / 4)) + (ano - 1970) * 365 + mthb[month-1] + (day - 1)) * 86400 + horas * 3600 + minute * 60 + second:
  ((((unsigned long int)( ano - 1970) / 4)) + (ano - 1970) * 365 + mth [month-1] + (day - 1)) * 86400 + horas * 3600 + minute * 60 + second;
  return timestamp;
}

