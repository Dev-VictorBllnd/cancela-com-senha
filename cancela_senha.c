#include <Keypad.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// ===== LCD I2C =====


LiquidCrystal_I2C lcd(0x20, 16, 2);


// ===== PINOS =====


// Sensores infravermelhos
#define IR_ENTRADA 10
#define IR_SAIDA 11


// LEDs
#define LED_VERDE 12
#define LED_VERMELHO 13


// Servo
#define SERVO_PIN A0


// Buzzer
#define BUZZER_PIN A1


// ===== TECLADO 4X4 =====


const byte LINHAS = 4;
const byte COLUNAS = 4;


char teclas[LINHAS][COLUNAS] = {
  {'1', '2', '3', 'X'},
  {'4', '5', '6', 'X'},
  {'7', '8', '9', 'X'},
  {'*', '0', '#', 'X'}
};


byte pinosLinhas[LINHAS] = {9, 8, 7, 6};
byte pinosColunas[COLUNAS] = {5, 4, 3, 2};


Keypad teclado = Keypad(
  makeKeymap(teclas),
  pinosLinhas,
  pinosColunas,
  LINHAS,
  COLUNAS
);


// ===== SERVO =====


Servo cancela;


const int PORTAO_FECHADO = 0;
const int PORTAO_ABERTO = 90;


// ===== SENHA =====


String senhaCorreta = "1234";
String senhaDigitada = "";


// ===== CONTROLE =====


bool portaoAberto = false;
bool veiculoDetectado = false;


unsigned long tempoAbertura = 0;
unsigned long ultimaDeteccao = 0;


const unsigned long TEMPO_MAXIMO = 10000;
const unsigned long TEMPO_FECHAMENTO = 1500;


// ===== SONS DO BUZZER =====


void somConfirmacao() {
  tone(BUZZER_PIN, 1500, 150);
  delay(180);
  tone(BUZZER_PIN, 2000, 150);
  delay(180);
  noTone(BUZZER_PIN);
}


void somErro() {
  tone(BUZZER_PIN, 300, 500);
  delay(500);
  noTone(BUZZER_PIN);
}


void somPassagem() {
  tone(BUZZER_PIN, 1200, 200);
  delay(220);
  noTone(BUZZER_PIN);
}


// ===== TELA INICIAL =====


void telaInicial() {


  lcd.clear();


  lcd.setCursor(0, 0);
  lcd.print("ESTACIONAMENTO");


  lcd.setCursor(0, 1);
  lcd.print("Digite a senha:");
}


// ===== MOSTRAR SENHA =====


void mostrarSenha() {


  lcd.setCursor(0, 1);
  lcd.print("Senha:          ");


  lcd.setCursor(6, 1);


  for (int i = 0; i < senhaDigitada.length(); i++) {
    lcd.print("*");
  }
}


// ===== ABRIR PORTAO =====


void abrirPortao() {


  if (!portaoAberto) {


    cancela.write(PORTAO_ABERTO);


    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);


    portaoAberto = true;
    veiculoDetectado = false;


    tempoAbertura = millis();


    lcd.clear();


    lcd.setCursor(0, 0);
    lcd.print("ACESSO LIBERADO");


    lcd.setCursor(0, 1);
    lcd.print("Portao aberto");


    Serial.println("Portao aberto!");


    somConfirmacao();
  }
}


// ===== FECHAR PORTAO =====


void fecharPortao() {


  cancela.write(PORTAO_FECHADO);


  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);


  portaoAberto = false;
  veiculoDetectado = false;


  lcd.clear();


  lcd.setCursor(0, 0);
  lcd.print("PORTAO FECHADO");


  lcd.setCursor(0, 1);
  lcd.print("Aguardando...");


  Serial.println("Portao fechado!");
}


// ===== VERIFICAR TECLADO =====


void lerTeclado(char tecla) {


  // Apagar senha
  if (tecla == '*') {


    senhaDigitada = "";


    mostrarSenha();


    return;
  }


  // Confirmar senha
  if (tecla == '#') {


    if (senhaDigitada.length() == 4 &&
        senhaDigitada == senhaCorreta) {


      Serial.println("Senha correta!");


      abrirPortao();


    } else {


      lcd.clear();


      lcd.setCursor(0, 0);
      lcd.print("SENHA INCORRETA");


      lcd.setCursor(0, 1);
      lcd.print("Tente novamente");


      Serial.println("Senha incorreta!");


      somErro();


      delay(1500);


      telaInicial();
    }


    senhaDigitada = "";


    return;
  }


  // Aceitar somente numeros
  if (tecla >= '0' && tecla <= '9') {


    if (senhaDigitada.length() < 4) {


      senhaDigitada += tecla;


      mostrarSenha();
    }
  }
}


// ===== SETUP =====


void setup() {


  Serial.begin(9600);


  // Inicializar LCD
  lcd.init();
  lcd.backlight();


  // Sensores
  pinMode(IR_ENTRADA, INPUT);
  pinMode(IR_SAIDA, INPUT);


  // LEDs
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);


  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);


  // Servo
  cancela.attach(SERVO_PIN);


  // Iniciar sistema
  fecharPortao();


  delay(1000);


  telaInicial();


  Serial.println("Sistema iniciado!");
  Serial.println("Digite a senha: 1234");
}


// ===== LOOP PRINCIPAL =====


void loop() {


  // Ler teclado
  char tecla = teclado.getKey();


  if (tecla) {


    // Ignorar a quarta coluna
    if (tecla != 'X' && !portaoAberto) {


      lerTeclado(tecla);
    }
  }


  // Ler sensores
  bool entrada = digitalRead(IR_ENTRADA) == LOW;
  bool saida = digitalRead(IR_SAIDA) == LOW;


  // Abrir automaticamente na deteccao da saida
  if (saida && !portaoAberto) {


    lcd.clear();


    lcd.setCursor(0, 0);
    lcd.print("VEICULO SAINDO");


    Serial.println("Veiculo detectado na saida!");


    abrirPortao();
  }


  // Controle do portao aberto
  if (portaoAberto) {


    // Detectar passagem
    if (entrada || saida) {


      if (!veiculoDetectado) {


        somPassagem();


        lcd.clear();


        lcd.setCursor(0, 0);
        lcd.print("VEICULO");


        lcd.setCursor(0, 1);
        lcd.print("DETECTADO!");
      }


      veiculoDetectado = true;


      ultimaDeteccao = millis();
    }


    // Fechar depois da passagem
    if (veiculoDetectado && !entrada && !saida) {


      if (millis() - ultimaDeteccao >= TEMPO_FECHAMENTO) {


        fecharPortao();
      }
    }


    // Fechar por tempo limite
    if (!veiculoDetectado &&
        millis() - tempoAbertura >= TEMPO_MAXIMO) {


      fecharPortao();
    }
  }


  // Voltar para a tela inicial quando fechado
  static bool telaAtualizada = false;


  if (!portaoAberto && !telaAtualizada) {


    telaInicial();


    telaAtualizada = true;
  }


  if (portaoAberto) {


    telaAtualizada = false;
  }
}
