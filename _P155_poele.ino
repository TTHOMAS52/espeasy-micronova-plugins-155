#include "_Plugin_Helper.h"
#include <SoftwareSerial.h>

#ifdef USES_P155

// #######################################################################################################
// #################################### Plugin 155: micronova                                    #########
// #######################################################################################################
/* 
 allumer le poele :http://ip/control?cmd=dummyvalueset,numero_task,numero_variable,-1

pour ma carte :
               ---- lecture  ram:0x00
               -----lecture  rom:0x20
               -----ecriture  ram:0x80
               -----ecriture  rom:0xA0
               

*/
// Maxim Integrated (ex Dallas) DS18B20 datasheet : https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
# include "src/Helpers/_Plugin_Helper_serial.h"
# include "src/PluginStructs/P155_data_struct.h"

//CONFIG_PIN1  Plugin_155_MicronovaPin_RX 
//CONFIG_PIN2  Plugin_155_MicronovaPin_TX 
//CONFIG_PIN3  Plugin_155_MicronovaPin_ENABLE_RX 
#define P155_TAILLE_ADRESSE 4

#define PLUGIN_155
#define PLUGIN_ID_155 4
#define PLUGIN_NAME_155 "MIcronova stove"
#define PLUGIN_VALUENAME1_155 "FONCTION"
#define P155_SENSOR_TYPE_INDEX 0
#define P155_NR_OUTPUT_VALUES getValueCountFromSensorType(static_cast<Sensor_VType>(PCONFIG(P155_SENSOR_TYPE_INDEX)))
#define P155_SERIAL_CONFIG PCONFIG(1)
#define P155_RX_BUFFER PCONFIG(2)
#define P155_DEFAULT_BAUDRATE 1200
#define P155_DEFAULT_RX_BUFFER 32
#define P155_BAUDRATE PCONFIG(3)
SoftwareSerial StoveSerial;
  int choix;
struct Code_Ram_Rom_Read_Write code_read_write_where = {0, 0, 0, 0};

uint8_t serialconfig;

uint32_t Value_Baud[10] = {110, 220, 300, 1200, 2400, 9600, 19200, 38400, 57600, 115200};

boolean success = false;

boolean On_OFF_ASK[4] = {false, false, false, false};
boolean Value_User_Change[4] = {false, false, false, false};
uint8_t index_ROM = 10;
uint8_t index_Adresse = 14;
uint8_t index_Ecriture = 18;
uint8_t index_Option = 22;
uint8_t index_repetition_commande = 26;

uint8_t choix_lecture_ecriture_ram_rom[4], Status_Poele[4];

String Plugin_155_valuename(uint8_t value_nr, bool displayString)
{
  String name = F("FONCTION");

  if (value_nr != 0)
    name += String(value_nr + 1);
  if (!displayString)
    name.toLowerCase();
  return name;
}

boolean Plugin_155(uint8_t function, struct EventStruct *event, String &string)
{

  boolean success = false;
  switch (function)
  {
  case PLUGIN_DEVICE_ADD:
  {
    Device[++deviceCount].Number = PLUGIN_ID_155;
    Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
    Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_QUAD;
    Device[deviceCount].Ports = 0;

    Device[deviceCount].PullUpOption = false;
    Device[deviceCount].InverseLogicOption = false;
    Device[deviceCount].FormulaOption = true;
    Device[deviceCount].DecimalsOnly = true;
    Device[deviceCount].ValueCount = 4;
    Device[deviceCount].SendDataOption = true;
    Device[deviceCount].TimerOption = true;
    Device[deviceCount].TimerOptional = true;
    Device[deviceCount].GlobalSyncOption = true;
    Device[deviceCount].OutputDataType = Output_Data_type_t::Simple;
    break;
  }

  case PLUGIN_GET_DEVICENAME:
  {
    string = F(PLUGIN_NAME_155);
    break;
  }

  case PLUGIN_GET_DEVICEVALUENAMES:
  {
    for (uint8_t i = 0; i < VARS_PER_TASK; ++i)
    {
      if (i < P155_NR_OUTPUT_VALUES)
      {
        safe_strncpy(
            ExtraTaskSettings.TaskDeviceValueNames[i],
            Plugin_155_valuename(i, false),
            sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
        ExtraTaskSettings.TaskDeviceValueDecimals[i] = 2;
      }
      else
      {
        ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
      }
    }
    break;
  }

  case PLUGIN_GET_DEVICEVALUECOUNT:
  {
    event->Par1 = P155_NR_OUTPUT_VALUES;
    success = true;
    break;
  }

  case PLUGIN_GET_DEVICEVTYPE:
  {
    event->sensorType = static_cast<Sensor_VType>(PCONFIG(P155_SENSOR_TYPE_INDEX));
    event->idx = P155_SENSOR_TYPE_INDEX;
    success = true;
    break;
  }

  case PLUGIN_SET_DEFAULTS:
  {

    //      P155_BAUDRATE         = P155_DEFAULT_BAUDRATE;
    //    P155_RX_BUFFER        = P155_DEFAULT_RX_BUFFER;

    PCONFIG(P155_SENSOR_TYPE_INDEX) = static_cast<uint8_t>(Sensor_VType::SENSOR_TYPE_SINGLE);
    for (uint8_t i = 0; i < VARS_PER_TASK; ++i)
    {
      ExtraTaskSettings.TaskDeviceValueDecimals[i] = 2;
    }

    success = true;
    break;
  }

  case PLUGIN_GET_DEVICEGPIONAMES:
  {
    // definition des variables
    const __FlashStringHelper *options_baud[10] =
        {F("110"), F("220"), F("300"), F("1200"), F("2400"), F("9600"), F("19200"),
         F("38400"), F("57600"), F("115200")

        };
    uint8_t Reponse_baud = PCONFIG(30*event->BaseVarIndex+3);

    const __FlashStringHelper *options_buffer[3] =
        {F("32"), F("64"), F("128")};
    int options_valeur_buffer[3] = {32, 64, 128};
    int Reponse_buffer = P155_RX_BUFFER;

    int Valeur_parity[40] =
        {
            SWSERIAL_5N1, SWSERIAL_6N1, SWSERIAL_7N1, SWSERIAL_8N1, SWSERIAL_5E1, SWSERIAL_6E1, SWSERIAL_7E1,
            SWSERIAL_8E1, SWSERIAL_5O1, SWSERIAL_6O1, SWSERIAL_7O1, SWSERIAL_8O1, SWSERIAL_5M1, SWSERIAL_6M1,
            SWSERIAL_7M1, SWSERIAL_8M1, SWSERIAL_5S1, SWSERIAL_6S1, SWSERIAL_7S1, SWSERIAL_8S1, SWSERIAL_5N2,
            SWSERIAL_6N2, SWSERIAL_7N2, SWSERIAL_8N2, SWSERIAL_5E2, SWSERIAL_6E2, SWSERIAL_7E2, SWSERIAL_8E2,
            SWSERIAL_5O2, SWSERIAL_6O2, SWSERIAL_7O2, SWSERIAL_8O2, SWSERIAL_5M2, SWSERIAL_6M2, SWSERIAL_7M2,
            SWSERIAL_8M2, SWSERIAL_5S2, SWSERIAL_6S2, SWSERIAL_7S2, SWSERIAL_8S2};

          const __FlashStringHelper *options_SERIAL_Parte[40] =
        {
            F("SWSERIAL_5N1"), F("SWSERIAL_6N1"), F("SWSERIAL_7N1"), F("SWSERIAL_8N1"), F("SWSERIAL_5E1"),
            F("SWSERIAL_6E1"), F("SWSERIAL_7E1"), F("SWSERIAL_8E1"), F("SWSERIAL_5O1"), F("SWSERIAL_6O1"),
            F("SWSERIAL_7O1"), F("SWSERIAL_8O1"), F("SWSERIAL_5M1"), F("SWSERIAL_6M1"), F("SWSERIAL_7M1"),
            F("SWSERIAL_8M1"), F("SWSERIAL_5S1"), F("SWSERIAL_6S1"), F("SWSERIAL_7S1"), F("SWSERIAL_8S1"),
            F("SWSERIAL_5N2"), F("SWSERIAL_6N2"), F("SWSERIAL_7N2"), F("SWSERIAL_8N2"), F("SWSERIAL_5E2"),
            F("SWSERIAL_6E2"), F("SWSERIAL_7E2"), F("SWSERIAL_8E2"), F("SWSERIAL_5O2"), F("SWSERIAL_6O2"),
            F("SWSERIAL_7O2"), F("SWSERIAL_8O2"), F("SWSERIAL_5M2"), F("SWSERIAL_6M2"), F("SWSERIAL_7M2"),
            F("SWSERIAL_8M2"), F("SWSERIAL_5S2"), F("SWSERIAL_6S2"), F("SWSERIAL_7S2"), F("SWSERIAL_8S2")};

    
    int reponse_config = PCONFIG(30*event->BaseVarIndex+1);

    // Partie configuration et affichage dans le bandeau sensor

    addFormNote(F(" affichage serialconfig"));
    addFormNote(String(static_cast<SoftwareSerialConfig>(PCONFIG(30*event->BaseVarIndex +1))));
    addFormNote(F("index du plugin"));

    addFormNote(String(event->TaskIndex));

    event->String1 = formatGpioName_input(F("RX"));
    event->String2 = formatGpioName_output(F("TX"));
    event->String3 = formatGpioName_output(F("Enable RX"));
    addFormNote(String(CONFIG_PIN2));

    addFormSelector(F("Baud_Rate"), F("P155_baud"), 10, options_baud, NULL,
                    Reponse_baud);

    addFormSelector(F("Serial_Parity"), F("SERIAL_PARITY"), 40, options_SERIAL_Parte,
                    Valeur_parity, reponse_config);

    addFormSelector(F("Serial_Rx_Buffer"), F("Serial_Rx_Buffer"), 3, options_buffer,
                    options_valeur_buffer, Reponse_buffer);

    break;
  }

  case PLUGIN_WEBFORM_LOAD:
  {

    serialHelper_getSerialTypeLabel(event);

    int8_t Plugin_155_MicronovaPin_RX = CONFIG_PIN1;
    int8_t Plugin_155_MicronovaPin_TX = CONFIG_PIN2;
    int8_t Plugin_155_MicronovaPin_ENABLE_RX = CONFIG_PIN3;
    addFormSubHeader(F("Configuration des fonctions"));

    if (Plugin_155_MicronovaPin_TX == -1)
    {
      Plugin_155_MicronovaPin_TX = Plugin_155_MicronovaPin_RX;
    }

    {

      const String P155_HEX_INPUT_PATTERN = F("(0x)?[0-9a-fA-F]{0,16}"); // 16 nibbles = 64 bit, 0x prefix is allowed but not added by
                                                                         // default
      String strCode;
      strCode.reserve(20);
      // if (nullptr != P155_data)
      addFormSubHeader(F("Configuration des codes ROM RAM pour la lecture et l'écriture"));
      addRowLabel(F("Code RAM_ROM"));
      html_table(EMPTY_STRING, false); // Sub-table
      html_table_header(F("Code Lecture RAM"));
      html_table_header(F("Code Lecture ROM"));
      html_table_header(F("Code Ecriture RAM"));
      html_table_header(F("Code Ecriture ROM"));
      html_TR_TD();
      addTextBox(F("code_Read_RAM"),
                 Convertir_Chaine_decimal_hexa(PCONFIG(30*event->BaseVarIndex+4)),
                 P155_TAILLE_ADRESSE, false, false, P155_HEX_INPUT_PATTERN);
      html_TD();
      addTextBox(F("code_Read_ROM"),
                 Convertir_Chaine_decimal_hexa(PCONFIG(30*event->BaseVarIndex+5)),
                 P155_TAILLE_ADRESSE, false, false, P155_HEX_INPUT_PATTERN);
      html_TD();
      addTextBox(F("code_Write_RAM"),
                 Convertir_Chaine_decimal_hexa(PCONFIG(30*event->BaseVarIndex+6)),
                 P155_TAILLE_ADRESSE, false, false, P155_HEX_INPUT_PATTERN);
      html_TD();
      addTextBox(F("code_Write_ROM"),
                 Convertir_Chaine_decimal_hexa(PCONFIG(30*event->BaseVarIndex+7)),
                 P155_TAILLE_ADRESSE, false, false, P155_HEX_INPUT_PATTERN);

      html_end_table();

      
      addFormSubHeader(F("Configuration Pour le status du poele"));

      addRowLabel(F("Statut poele"));
      html_table(EMPTY_STRING, false); // Sub-table
      html_table_header(F("Adresse Status Poele"));
      html_table_header(F("Localiser du status dans RAM_ROM"));
      html_TR_TD();
      addTextBox(F("Status du poele"),
                 Convertir_Chaine_decimal_hexa(PCONFIG(30*event->BaseVarIndex+8)),
                 P155_TAILLE_ADRESSE, false, false, P155_HEX_INPUT_PATTERN, F(""));
      html_TD();

      addCheckBox(F("Status_Ram_ROM"), PCONFIG(30*event->BaseVarIndex+9) == 1, false);

      html_end_table();

      addFormSubHeader(F("Configuration des fonctions"));
      addRowLabel(F("POELE"));
      html_table(EMPTY_STRING, false); // Sub-table
      html_table_header(F("FONCTION"));
      html_table_header(F("ROM"));
      html_table_header(F("Ecriture"));
      html_table_header(F("Adresse"));
      html_table_header(F("Valeur_ON_OFF"));
      html_table_header(F("Répétition"));

      for (uint8_t i = 0; i < P155_NR_OUTPUT_VALUES; ++i)
      {
        String DeviceName = String(ExtraTaskSettings.TaskDeviceValueNames[i]);
        addFormNote(String(choix_lecture_ecriture_ram_rom[i]));
        html_TR_TD();
        addFormCheckBox(DeviceName, DeviceName + F("ROM"), PCONFIG(30*event->BaseVarIndex+index_ROM + i) == 1, false);
        html_TD();
        addCheckBox(DeviceName + F("Ecriture"), PCONFIG(30*event->BaseVarIndex+index_Ecriture + i) == 1, false);
        html_TD();
        addTextBox(DeviceName + F("Adresse"),
                   Convertir_Chaine_decimal_hexa(PCONFIG(30*event->BaseVarIndex+index_Adresse + i)), P155_TAILLE_ADRESSE, false, false, P155_HEX_INPUT_PATTERN, F(""));

        if ((DeviceName == F("On")) || (DeviceName == F("Off")))
        {
          html_TD();
          addTextBox(DeviceName + F("VALEUR_ON_OFF"),
                     Convertir_Chaine_decimal_hexa(PCONFIG(30*event->BaseVarIndex+index_Option + i)),
                     P155_TAILLE_ADRESSE, false, false, P155_HEX_INPUT_PATTERN, F(""));
          html_TD();
          addCheckBox(DeviceName + F("repetition"), PCONFIG(30*event->BaseVarIndex+index_repetition_commande + i) == 1, false);
        }
      }
      html_end_table();
    }

    success = true;
    break;
  }
  case PLUGIN_WEBFORM_SAVE:
  {
  
    PCONFIG(30*event->BaseVarIndex+1) = getFormItemInt(F("SERIAL_PARITY"));
    PCONFIG(30*event->BaseVarIndex+2) = getFormItemInt(F("Serial_Rx_Buffer"));
    PCONFIG(30*event->BaseVarIndex+3) = getFormItemInt(F("P155_baud"));
    PCONFIG(30*event->BaseVarIndex+4) = getFormItemInt(F("code_Read_RAM"));
    PCONFIG(30*event->BaseVarIndex+5) = getFormItemInt(F("code_Read_ROM"));
    PCONFIG(30*event->BaseVarIndex+6) = getFormItemInt(F("code_Write_RAM"));
    PCONFIG(30*event->BaseVarIndex+7) = getFormItemInt(F("code_Write_ROM"));
    PCONFIG(30*event->BaseVarIndex+8) = getFormItemInt(F("Status du poele"));
    PCONFIG(30*event->BaseVarIndex+9) = isFormItemChecked(F("Status_Ram_ROM"));
    code_read_write_where = {PCONFIG(30*event->BaseVarIndex+4), PCONFIG(30*event->BaseVarIndex+5), 
    PCONFIG(30*event->BaseVarIndex+6), PCONFIG(30*event->BaseVarIndex+7)};
    int8_t Plugin_155_MicronovaPin_RX = CONFIG_PIN1;
    int8_t Plugin_155_MicronovaPin_TX = CONFIG_PIN2;
    int8_t Plugin_155_MicronovaPin_ENABLE_RX = CONFIG_PIN3;

    if (Plugin_155_MicronovaPin_TX == -1)
      Plugin_155_MicronovaPin_TX = Plugin_155_MicronovaPin_RX;

    for (uint8_t i = 0; i < P155_NR_OUTPUT_VALUES; ++i)
    {
      choix = 0;
      String DeviceName = String(ExtraTaskSettings.TaskDeviceValueNames[i]);
      PCONFIG(30*event->BaseVarIndex+index_ROM + i) = isFormItemChecked(DeviceName + F("ROM"));
      PCONFIG(30*event->BaseVarIndex+index_Adresse + i) = getFormItemInt(DeviceName + F("Adresse"));
      PCONFIG(30*event->BaseVarIndex+index_Ecriture + i) = isFormItemChecked(DeviceName + F("Ecriture"));
      PCONFIG(30*event->BaseVarIndex+index_repetition_commande + i) = isFormItemChecked(DeviceName + F("repetition"));
       
      if(PCONFIG(30*event->BaseVarIndex+9)) Status_Poele[i] =1;
      else Status_Poele[i] =0 ;
          if(PCONFIG(30*event->BaseVarIndex+index_Ecriture + i)) { choix = 10;}
          if(PCONFIG(30*event->BaseVarIndex+index_ROM + i)) {choix += 1 ;}
  

      choix_lecture_ecriture_ram_rom[i] = choix;
      choix = 0;
      if ((DeviceName == F("On")) || (DeviceName == F("Off")))
        PCONFIG(30*event->BaseVarIndex+index_Option + i) = getFormItemInt(DeviceName + F("VALEUR_ON_OFF"));
    }

    success = true;
    break;
  }

  case PLUGIN_WEBFORM_SHOW_CONFIG:
  {
    LoadTaskSettings(event->TaskIndex);

    for (uint8_t i = 0; i < VARS_PER_TASK; ++i)
      if (i < P155_NR_OUTPUT_VALUES)
        if (i != 0)
          string += F("<br>");

    success = true;
    break;
  }

  case PLUGIN_INIT:
  {

        code_read_write_where = {PCONFIG(30*event->BaseVarIndex+4), PCONFIG(30*event->BaseVarIndex+5), 
                                 PCONFIG(30*event->BaseVarIndex+6), PCONFIG(30*event->BaseVarIndex+7)};

          for (uint8_t i = 0; i < P155_NR_OUTPUT_VALUES; ++i){

                if(PCONFIG(30*event->BaseVarIndex+index_Ecriture + i)) { choix = 10;}
                if(PCONFIG(30*event->BaseVarIndex+index_ROM + i)) {choix += 1 ;}
                if(PCONFIG(30*event->BaseVarIndex+9)) Status_Poele[i] =1;
                     else Status_Poele[i] =0 ;

                choix_lecture_ecriture_ram_rom[i] = choix;
                choix = 0;
          }

     P155_data_struct *P155_data =
       static_cast<P155_data_struct *>(getPluginTaskData(event->TaskIndex));
    LoadTaskSettings(event->TaskIndex);

    int8_t Plugin_155_MicronovaPin_RX = CONFIG_PIN1;
    int8_t Plugin_155_MicronovaPin_TX = CONFIG_PIN2;
    int8_t Plugin_155_MicronovaPin_ENABLE_RX = CONFIG_PIN3;
    //delay(10e3);
    pinMode(Plugin_155_MicronovaPin_ENABLE_RX, OUTPUT);
    digitalWrite(Plugin_155_MicronovaPin_ENABLE_RX, HIGH);
    int baud = P155_BAUDRATE;

    String log = F("Initialisation port serie:");
    log += F("RX =");
    log += CONFIG_PIN1;
    log += F("_____TX =");
    log += CONFIG_PIN2;
    log += F("ENABLE RX =");
    log += CONFIG_PIN3;
    log += F("__valeur baud =");
    log += log += Value_Baud[baud];

    addLog(LOG_LEVEL_INFO, log);

    if ((validGpio(Plugin_155_MicronovaPin_RX)) && (validGpio(Plugin_155_MicronovaPin_TX)))
    {

      // initialisation de sofwreserial
      StoveSerial.enableIntTx(false);
      StoveSerial.begin(Value_Baud[baud], static_cast<SoftwareSerialConfig>(PCONFIG(30*event->BaseVarIndex+1)),
                        Plugin_155_MicronovaPin_RX, Plugin_155_MicronovaPin_TX, false,
                        P155_RX_BUFFER, P155_RX_BUFFER);

      delay(2000);

      

        success = true;
      
    }
    break;
  }

  case PLUGIN_READ:
  {

    
    int8_t Plugin_155_MicronovaPin_RX = CONFIG_PIN1;
    int8_t Plugin_155_MicronovaPin_TX = CONFIG_PIN2;
    int8_t Plugin_155_MicronovaPin_ENABLE_RX = CONFIG_PIN3;
 String log;
    
    for (uint8_t i = 0; i < VARS_PER_TASK; ++i)
    {
      if (i < P155_NR_OUTPUT_VALUES)
      {

        if (PCONFIG(30*event->BaseVarIndex+index_Ecriture + i)==1) //
        {
          if ( String(ExtraTaskSettings.TaskDeviceValueNames[i]) ==String("On")) 
          { log = F("je suis dans ecriture On");
                 addLog(LOG_LEVEL_INFO, log);

                   uint8_t temp ;  
                   delay (80);
                   temp = (interroge_ram_eprom_poele(
                   Add_Ram_Rom_Read_write(1, code_read_write_where), PCONFIG(30*event->BaseVarIndex+8),
                               Plugin_155_MicronovaPin_ENABLE_RX));
            
            UserVar[(event->BaseVarIndex + i)] = temp;
            On_OFF_ASK[i] = false;
          }
          
        }
        else
        {
          UserVar[(event->BaseVarIndex + i)] = interroge_ram_eprom_poele(
              Add_Ram_Rom_Read_write(choix_lecture_ecriture_ram_rom[i], code_read_write_where),
              PCONFIG(30*event->BaseVarIndex+index_Adresse + i), Plugin_155_MicronovaPin_ENABLE_RX);
               log = F("je suis en mode lecture");
                 addLog(LOG_LEVEL_INFO, log);
        }
      }
      On_OFF_ASK[i] = false;
      delay(200);
    }

    success = true;
    break;
  }

  case PLUGIN_WRITE:
  {
    
    int8_t Plugin_155_MicronovaPin_RX = CONFIG_PIN1;
    int8_t Plugin_155_MicronovaPin_TX = CONFIG_PIN2;
    int8_t Plugin_155_MicronovaPin_ENABLE_RX = CONFIG_PIN3;

    String log;
    String command = parseString(string, 1);
    if (command == F("dummyvalueset"))
    {
      if (event->Par1 == event->TaskIndex + 1) // make sure that this instance is the target
      {
        float floatValue = 0;

        if (string2float(parseString(string, 4), floatValue))
        {
          if (loglevelActiveFor(LOG_LEVEL_INFO))
          {
            String log = F("Dummy: Index ");
            log += event->Par1;
            log += F(" value ");
            log += event->Par2;
          }

                 int   var_y = static_cast<int>(floatValue);

            switch (var_y)
            {
            case -1:              On_OFF_ASK[event->Par2 - 1] = true;
                                  Ecriture_ram_eprom_poele(
                                         Add_Ram_Rom_Read_write(choix_lecture_ecriture_ram_rom[event->Par2 - 1], code_read_write_where)
                                         , PCONFIG(30*event->BaseVarIndex+event->Par2 - 1+index_Adresse), 
                                         PCONFIG(30*event->BaseVarIndex+index_Option + event->Par2 - 1 ), Plugin_155_MicronovaPin_ENABLE_RX);

                                      Value_User_Change[event->Par2 - 1] = false;
                                      UserVar[event->BaseVarIndex + event->Par2 - 1] = 10;
                    break;
            case -2:              On_OFF_ASK[event->Par2 - 1] = true;
                                  Value_User_Change[event->Par2 - 1] = false;

                                          UserVar[event->BaseVarIndex + event->Par2 - 1] = 33;
                    break;
            
            default:                                           
                              String DeviceName = String(ExtraTaskSettings.TaskDeviceValueNames[event->Par2 - 1]);
                                  On_OFF_ASK[event->Par2 - 1] = false;
                                  Value_User_Change[event->Par2 - 1] = true;
                                 if (UserVar[event->BaseVarIndex + event->Par2 - 1] 
                                    != floatValue  )
                                  UserVar[event->BaseVarIndex + event->Par2 - 1] = floatValue;
                                  Ecriture_ram_eprom_poele(
                                    
                                    Add_Ram_Rom_Read_write(choix_lecture_ecriture_ram_rom[event->Par2 - 1], code_read_write_where)
                                   , PCONFIG(30*event->BaseVarIndex+event->Par2 - 1+index_Adresse) 
                                   , floatValue,
                                    Plugin_155_MicronovaPin_ENABLE_RX);
                                  


              break;
            }

          success = true;
        }
        else
        { // float conversion failed!
          if (loglevelActiveFor(LOG_LEVEL_ERROR))
          {
            log = F("Dummy: Index ");
            log += event->Par1;
            log += F(" value ");
            log += event->Par2;
            log += F(" parameter3: ");
            log += parseStringKeepCase(string, 4);
            log += F(" not a float value!");
            addLog(LOG_LEVEL_ERROR, log);
          }
        }
      
      
      
      
      
      
      
      
      
      
      
      
      
      }
    }

    break;
  }
  }
  return success;
}

#endif // USES_P155

byte Add_Ram_Rom_Read_write(int code_choisi, struct Code_Ram_Rom_Read_Write code)
{
  byte val = 1;
  String log;

  switch (code_choisi)
  {
  case 0:
    val = code.Code_Read_RAM;
    break;

  case 1:
    val = code.Code_Read_ROM;
    break;

  case 10:
    val = code.Code_Write_RAM;
    break;

  case 11:
    val = code.Code_Write_ROM;
    break;
  }

 // log = F("acces memoire  = ");
 // log += Convertir_Chaine_decimal_hexa(val);
 // addLog(LOG_LEVEL_INFO, log);

  return val;
}

void Ecriture_ram_eprom_poele(byte Type, byte Addr, byte Data, uint8_t enable_rx)
{                                         // ecriture dans la ram ou rom dans le poele (type = 0x80 pour ram, 0xA2 pour la romm
  byte cs = ((Type + Addr + Data) % 256); // (checksum pour verifier l'envoie des donner  (somme de type, addresse,valeur modulo 256
  StoveSerial.write(Type);
  StoveSerial.write(Addr);
  StoveSerial.write(Data);
  StoveSerial.write(cs);
  digitalWrite(enable_rx, LOW);
}







byte lit_reponse_poele(uint8_t enable_rx)
{ // lecture de la reponse du Poele
  uint8_t rxCount = 0;
  byte val = 0;
  char stoveRxData[2];

  stoveRxData[0] = 0x00;
  stoveRxData[1] = 0x00;
  while (StoveSerial.available())
  {
    stoveRxData[rxCount] = StoveSerial.read();
    rxCount++;
  }

  digitalWrite(enable_rx, HIGH);
  if (rxCount == 2)
    val = stoveRxData[1];
  return val;
}

byte interroge_ram_eprom_poele(byte ram_eprom, byte adresse, uint8_t enable_rx)
{ // demande la valeur de la ram ou rom à la adresse
  String log;
  uint8_t reponse_poele;
  StoveSerial.write(ram_eprom);
  StoveSerial.write(adresse);
  digitalWrite(enable_rx, LOW);
  delay(100);
  reponse_poele = lit_reponse_poele(enable_rx);
  log = F("RAM ROM ECRTITURE LECTURE");
  log += ram_eprom;
  log += F("Adresse");
  log += adresse;
  log += F("reponse = ");
  log += reponse_poele;

  addLog(LOG_LEVEL_INFO, log);

  return reponse_poele;
}

int x2i(char *s)
{ // Convertir un tableau de caractere en valeur hexa
  int x = 0;
  for (;;)
  {
    char c = *s;
    if (c >= '0' && c <= '9')
    {
      x *= 16;
      x += c - '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
      x *= 16;
      x += (c - 'A') + 10;
    }
    else
      break;
    s++;
  }
  return x;
}

int conversion_entier_hex(uint8_t valeur)
{ // convertie un entier <256 en valeur hexa
  String Chaine = "";
  char valeur_char[3];
  int conversion;
  int quotient, reste;
  reste = valeur % 16;
  quotient = (valeur - reste) / 16;
  Chaine = Convertir_hex_elementaire(quotient) + Convertir_hex_elementaire(reste);
  Chaine.toCharArray(valeur_char, 3);
  conversion = x2i(valeur_char);
  return conversion;
}

String Convertir_Chaine_decimal_hexa(uint8_t Valeur)
{ // convertie un nombre  en hexa
  String Chaine;
  Chaine = "";
  uint8_t quotient;
  uint8_t reste;
  reste = Valeur % 16;
  quotient = (Valeur - reste) / 16;
  Chaine = "0x" + Convertir_hex_elementaire(quotient) + Convertir_hex_elementaire(reste);
  return Chaine;
}

String Convertir_hex_elementaire(uint8_t Valeur)
{ // convertie un nombre <=16 en hexa
  String Chaine;

  if (Valeur <= 9)
    Chaine = String(Valeur);
  else
    switch (Valeur)
    {
    case 10:
      Chaine = "A";
      break;

    case 11:
      Chaine = "B";
      break;

    case 12:
      Chaine = "C";
      break;

    case 13:
      Chaine = "D";
      break;

    case 14:
      Chaine = "E";
      break;

    case 15:
      Chaine = "F";
      break;
    }

  return Chaine;
}




////////////////////////////////////////////////////////////////////////////////////////////////////

/*

Si le nom est ON ou OFF --> ecriture dans le Status_Poele
Si le nom est different regarder si la fonction est bien en mode ecriture dans ram ou rom 




void modification_ram_rom_dans_poele(){

    String command = parseString(string, 1);
    if (command == F("changement_parametre"))
    {
      if (event->Par1 == event->TaskIndex + 1) // make sure that this instance is the target
      {
        float floatValue = 0;

        if (string2float(parseString(string, 4), floatValue))
        {

                 int   var_y = static_cast<int>(floatValue);

            
            
            
}
*/
