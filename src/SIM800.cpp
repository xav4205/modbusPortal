#include <Arduino.h>
#include <WebSerial.h>

#include "SIM800.h"
#include "config.h"

Sim800::Sim800()
{

  _sender = "";
  _bodySms = "";
  _isConnected = false;
  _operator = "";
  _isReady = false;

  _sim800WatchdogTimer = millis();

  return;
}

void Sim800::begin(int baudRate, int txPin, int rxPin)
{
  Serial1.begin(baudRate, SERIAL_8N1, txPin, rxPin);
  Serial1.setTimeout(TIME_OUT);

  bool isReady = false;
  Serial1.flush();
  int nb_start = 3;

  setEchoMode(true);

  while (!atCommand("AT", endAt::returnCarriage) && nb_start > 0)
  {
    WebSerial.println("Test de la carte Sim800");
    WebSerial.print("Essais restants :");
    WebSerial.println(nb_start);

    // Demarrage du shield
    delay(T_RESP * 5);
    nb_start--;
  }

  if (nb_start > 0)
  {
    isReady = true;
  }

  // Teste si le module est deja allumé? (Vidage du tampon de lecture, envoi d'une commande AT, attente de reponse)

  if (isReady)
  {

    WebSerial.println("Communication établie avec le module SIM800");
    WebSerial.println("Initialisation");
    Serial1.flush();

    // Configuration du shield

    sendPinCode();

    setTextModeSMS();

    deleteSms();

    requestNetworkRegistration();

    requestSignalQuality();

    WebSerial.println("Pret !");
  }
  else
  {
    WebSerial.println("Le shield Sim800 ne fonctionne pas");
  }
}

void Sim800::test(boolean reset)
{

  if (_sim800WatchdogTimer + GPRS_WATCHDOG < millis() || reset)
  {
    //gprs->atCommand("AT");
    WebSerial.println("GPRS WATCHDOG");
    if (atCommand("AT", endAt::returnCarriage) != 1)
    {
      WebSerial.println("Pas de reponse du module gprs. RESET!!!");
      setTextModeSMS();
      deleteSms();
    }
    _sim800WatchdogTimer = millis();
  }
}

/**
 * @brief Envoie un sms
 * 
 * @param recipient Numero du destinataire
 * @param text Corps du message
 */
void Sim800::sendSms(const String &recipient, const String &text)
{
  WebSerial.print("\nEnvoi du SMS au ");
  WebSerial.println(recipient);
  WebSerial.println(text);

  String header = "AT+CMGS=\"" + (recipient) + ("\"");

  atCommand(header, endAt::returnCarriage);
  atCommand(text, endAt::endMark);

  delay(5000);
  read();
}

/**
 * @brief Returne un message au dernier expediteur de message
 * 
 * @param text Corps du message
 */
void Sim800::sendToLastSender(const String &text)
{
  sendSms(_sender, text);
}

/**
 * @brief Lit le sms stocké en mémoire
 * 
 * @param SmsStorePos Position du message à lire dans la mémoire
 * @return Texte du dernier message 
 */
String Sim800::readSms(const String SmsStorePos)
{
  WebSerial.print("Lit le SMS numero ");
  WebSerial.println(SmsStorePos);

  String command = "AT+CMGR=" + SmsStorePos;

  atCommand(command, endAt::returnCarriage);

  // Extraction de l'expediteur

  String firstLine = _reply[1];
  if (firstLine.indexOf("+CMGR:") >= 0)
  {

    int cPos = firstLine.indexOf(',');
    _sender = firstLine.substring(cPos + 2, cPos + 14);

    WebSerial.print("SMS du numero :");
    WebSerial.println(_sender);
  }

  // Extraction du texte du sms

  _bodySms = _reply[2];

  WebSerial.println("============ MESSAGE ===============");

  WebSerial.println(_bodySms);

  WebSerial.println("=========================");

  return _bodySms;
}

/**
* @brief Envoi la commande AT au module, renvoie ok si le module repond bien (echo valide)
* @param at Commande AT
* @param end Defini si la commande AT doit finir par 'Enter' ou 'Ctrl+Z'
* @return Echo valide
*/
bool Sim800::atCommand(const String at, endAt end)
{
  unsigned long atTimeOut = millis();

  bool isOk = false;

  String response = "";

  if (DEBUG_AT)
  {
    WebSerial.println("==> COMMANDE AT ESP32>>SIM800 =>  " + at);
  }

  if (end == endAt::returnCarriage) // Envoi de la commande
    Serial1.println(at);
  else
  {
    Serial1.print(at);
    sendEndMark();
  }

  while (millis() - atTimeOut < AT_TIME_OUT) // Attente d'une reponse
  {
    if (Serial1.available() > 0) // Lecture et verification de la réponse
    {
      bool echo = read(at);

      if (echo)
        isOk = true;

      atTimeOut = millis();
      break;
    }
  }

  if (millis() - atTimeOut >= AT_TIME_OUT) // Pas de reponse
  {
    WebSerial.print("==> AT TIMEOUT <==");
  }

  return isOk;
}

/**
 * @brief Envoie le caractére de fin spécial pour validation du texte d'un SMS
 * 
 */
void Sim800::sendEndMark(void)
{
  Serial1.write((char)26);
  Serial1.println();
}

/**
 * @brief Lie le port série du module Sim800
 * 
 * @param at (optionnel) Commande AT à comparer à la réponse
 * @return Réponse identique au paramètre at
 */
bool Sim800::read(String at)
{
  bool echo = false;
  int nbOfLines = 0;

  while (Serial1.available() && nbOfLines < MAX_NB_REPLY)
  {

    String message = "";
    bool spontaneousMessage = at.length() <= 0;

    char c = ' ';
    while (Serial1.available() && c != '\r')
    {
      c = Serial1.read();
      if (c != (char)10) // Elimine les line feed
        message += c;
    }
    // read the incoming byte:

    String debug = "====> Message SIM800>>ESP32 (";

    at.trim();
    debug += spontaneousMessage ? "spontané) =>  " : "suite à [" + at + "]) =>";

    if (message.length() > 0)
    {
      message.trim();
      WebSerial.print(debug + message);

      _reply[nbOfLines] = message;

      if (nbOfLines == 0 && !spontaneousMessage && at == message)
      {
        WebSerial.println("\t=> ECHO VALIDE");
        echo = true;
      }
      else
        WebSerial.println();

      _messageProcess = spontaneousMessage;

      nbOfLines++;
    }

    delay(500);
  }

  // Efface la fin du buffer de reception
  for (int j = nbOfLines; j < MAX_NB_REPLY; j++)
  {
    _reply[j] = "";
  }

  // WebSerial.print("Nombre de lignes lues : ");
  // WebSerial.println(i);

  process();

  return echo;
}

/**
 * @brief Efface les sms stockés en mémoire
 * 
 */
void Sim800::deleteSms()
{
  WebSerial.println("=>Demande de suppression des SMS");
  atCommand("AT+CMGD=1,4", endAt::returnCarriage);
  return;
}

/**
 * @brief Permet au module de recevoir et d'envoyer des sms
 * 
 */
void Sim800::setTextModeSMS()
{
  WebSerial.println("=> Parametrage SMS mode Texte");
  atCommand("AT+CMGF=1", endAt::returnCarriage);
  return;
}

/**
 * @brief Demande de niveau de signal
 * 
 * @return niveau de reception (0 à 4 "barrettes") 
 */
int Sim800::requestSignalQuality()
{
  String s;
  int level = 0;

  WebSerial.println("=> Demande de la qualite de signal");

  if (atCommand("AT+CSQ", endAt::returnCarriage))
  {

    String header = _reply[1];

    if (header.startsWith("+CSQ"))
    {
      level = extractBetween(header, ": ", ",").toInt();

      s = "Niveau du signal : ";
      s += level;
      WebSerial.println(s);
    }
  }

  int bar = 0; // Nombre de barrettes
  if (level > 7)
    bar = 1;
  if (level > 10)
    bar = 2;
  if (level > 14)
    bar = 3;
  if (level > 19)
    bar = 4;

  return bar;
}

/**
 * @brief Verifie si le module est attaché au réseau
 * 
 * @return int 
 */
bool Sim800::requestNetworkRegistration()
{
  String s;
  int n = 0;

  WebSerial.println("=> Demande si le module est attaché au réseau");

  if (atCommand("AT+CGATT?", endAt::returnCarriage))
  {

    String header = _reply[1];

    if (header.startsWith("+AT+CGATT"))
    {
      n = extractBetween(header, ": ", "").toInt();
    }
  }

  return n;
}

/**
 * @brief Parametre le module pour qu'il renvoie la commande AT
 * 
 * @param activate ON/OFF
 */
void Sim800::setEchoMode(bool activate)
{

  WebSerial.println("=> Mode Echo");

  String s = activate ? "ATE1" : "ATE0";

  atCommand(s, endAt::returnCarriage);
}

/**
 * @brief Parametre le code PIN à 0000
 * 
 */
void Sim800::sendPinCode()
{
  WebSerial.println("=> Envoi du code PIN");
  char pin[] = ("AT+CPIN=0000");

  if (atCommand("AT+CPIN?", endAt::returnCarriage))
  {

    String firstLine = _reply[1]; // Extrait la premier trame apres l'echo

    if (firstLine.startsWith("+CPIN"))
    {
      if (firstLine.indexOf("READY") > 0)
      {
        WebSerial.println(" Pas besoin de code Pin");
      }
      else if (atCommand(pin, endAt::returnCarriage))
      {
        String newFirstLine = _reply[1]; // Extrait la premier trame apres l'echo
        if (newFirstLine.startsWith("+CPIN"))
        {
          if (String(_reply[2]).indexOf("OK") > 0)
          {
            WebSerial.println("Code Pin accepte");
          }
          else
            WebSerial.println("Mauvais Code Pin");
        }
      }
    }
  }
  return;
}

/*
String Sim800::surveySpontaneousCommand()
{
  // surveille les messages spontanes et le stocke en tampon dans _message Renvoie True si un nouveau message est detecte
  String message = "";
  message = read();
  boolean isEmpty = message.length() < 1;

  if (!isEmpty)
  {
    WebSerial.println("======= MESSAGE SPONTANE ========");
    WebSerial.println();
    WebSerial.println(message);
  }

  return message;
}
*/

boolean Sim800::isReady()
{
  return _isReady;
}

String Sim800::extractBetween(String s, String a, String b)
{
  int beginSub = s.indexOf(a) + a.length();
  int endSub = s.indexOf(b, beginSub);
  return s.substring(beginSub, endSub);
}

/**
 * @brief  Boucle principale :
 * - surveille les message spontanés
 * 
 */
void Sim800::run()
{
  if (Serial1.available() > 0)
  {
    WebSerial.println("*");
    read();
  }
}

void Sim800::process()
{

  if (_messageProcess)
  {
    if (DEBUG_RESPONSE)
    {
      WebSerial.println("Processing of message ...");
      WebSerial.println("--------------------------------------------------------------");
    }

    for (int i = 0; i < MAX_NB_REPLY; i++)
    {
      if (DEBUG_RESPONSE)
        WebSerial.println("|\t" + String(i, DEC) + "\t|\t" + _reply[i]);
      //_reply[i] = "";
    }
    if (DEBUG_RESPONSE)
      WebSerial.println("--------------------------------------------------------------");

    String header = _reply[0];

    if (header.startsWith("+CMTI:")) // si c'est un SMS
    {

      String position = header.substring(header.indexOf(",") + 1);
      WebSerial.print("SMS stocke en position ");
      WebSerial.println(position);

      readSms(position);
    }

    _messageProcess = false;
  }
}
