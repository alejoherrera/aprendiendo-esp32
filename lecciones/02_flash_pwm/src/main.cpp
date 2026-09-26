/**
 * Leccion 02 - LED flash con PWM.
 *
 * Controla el brillo del LED blanco grande (el "flash" de la camara) usando PWM,
 * y permite cambiarlo escribiendo un numero en el monitor serie.
 */

#include <Arduino.h>
#include <panel.h>

// --- Parte 1: pines, canal PWM y parametros ----------------------------------
// El flash esta en el GPIO 4 (logica normal: HIGH = encendido).
// OJO: el GPIO 4 tambien es una linea de datos de la microSD; si usas SD, el flash parpadea.
const int LED_FLASH = 4;

// El ESP32 genera PWM con el periferico LEDC: 16 canales repartidos en 4 temporizadores.
// La camara usa el canal 0 / temporizador 0 para su reloj (XCLK). Usamos el canal 4
// (temporizador 2) para que, cuando sumemos la camara, no se pisen.
const int CANAL_PWM = 4;
const int FRECUENCIA_HZ = 5000;
const int RESOLUCION_BITS = 12;  // 12 bits -> 4096 niveles, de 0 a 4095
const int PWM_MAXIMO = 4095;

// Brillo en PORCENTAJE (0-100 %) de lo que el ojo percibe. El flash es MUY brillante y se
// calienta: el tope se ajusta desde el panel. 50 % percibido ya es mucha luz.
const long& BRILLO_MAXIMO = panel::parametro("BRILLO_MAXIMO", 50, 1, 100, "Tope de brillo (% percibido)");
const long& PASO_MS = panel::parametro("PASO_MS", 20, 1, 100, "Velocidad del efecto respirar (ms por paso)");

// Recuerda que esta haciendo el flash: true = respira, false = brillo fijo.
bool respirando = true;

// --- Parte 2: una funcion por responsabilidad --------------------------------
// El ojo no ve la luz en proporcion: nota mucho los cambios en brillos bajos y casi nada en
// los altos (medido en esta placa: 2 % de PWM ya duplica la luz de la escena). La "correccion
// gamma" convierte el % que queremos VER en el valor de PWM que hay que MANDAR.
int porcentajeAPwm(int porcentaje) {
    float fraccion = porcentaje / 100.0;
    return round(PWM_MAXIMO * pow(fraccion, 2.2));
}

void ponerBrillo(int pedido) {
    int porcentaje = constrain(pedido, 0, (int)BRILLO_MAXIMO);
    int pwm = porcentajeAPwm(porcentaje);
    ledcWrite(CANAL_PWM, pwm);
    Serial.printf("Brillo: %d%% (PWM %d/4095)\n", porcentaje, pwm);
    if (porcentaje != pedido) {
        Serial.printf("  Pediste %d%%, pero el tope es BRILLO_MAXIMO = %ld%%. Subilo en Variables.\n",
                      pedido, BRILLO_MAXIMO);
    }
}

void respirar() {
    for (int p = 0; p <= BRILLO_MAXIMO; p++) {
        ledcWrite(CANAL_PWM, porcentajeAPwm(p));
        delay(PASO_MS);
    }
    for (int p = BRILLO_MAXIMO; p >= 0; p--) {
        ledcWrite(CANAL_PWM, porcentajeAPwm(p));
        delay(PASO_MS);
    }
}

// --- Parte 3: setup ------------------------------------------------------------
void setup() {
    Serial.begin(921600);
    delay(1000);
    panel::iniciar("02_flash_pwm");
    panel::parte(3);
    Serial.println("\nLeccion 02: flash con PWM");

    // API de Arduino core 2.x: primero se configura el canal, despues se le asigna el pin.
    ledcSetup(CANAL_PWM, FRECUENCIA_HZ, RESOLUCION_BITS);
    ledcAttachPin(LED_FLASH, CANAL_PWM);

    Serial.println("El flash respira. Escribi un porcentaje 0-100 para dejarlo fijo, o 'r' para volver a respirar.");
}

// --- Parte 4: loop: respirar o esperar ordenes -----------------------------------
void loop() {
    panel::parte(4);
    String linea;
    // leerLinea() entrega lo que escribiste, salvo los comandos del panel (PAUSA, SET...).
    if (panel::leerLinea(linea)) {
        if (linea == "r") {
            respirando = true;
        } else {
            respirando = false;
            ponerBrillo(linea.toInt());
        }
    }
    // Un ciclo de respiracion por vuelta: asi cada cambio de BRILLO_MAXIMO o PASO_MS
    // desde el panel se ve en el ciclo siguiente.
    if (respirando) respirar();
}
