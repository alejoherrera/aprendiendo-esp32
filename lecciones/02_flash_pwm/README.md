# Lección 02: el flash con PWM

**Objetivo:** controlar el brillo del LED blanco grande (el flash) con **PWM** y recibir
órdenes desde el monitor serie.

## Qué es PWM

Un pin digital solo puede estar en 0 V o 3.3 V. Para "medio brillo" se prende y apaga
muy rápido (5000 veces por segundo en esta lección): el ojo percibe el **promedio**.
El porcentaje del tiempo que está encendido se llama *duty cycle*.

```
PWM al 25 %   █▁▁▁█▁▁▁█▁▁▁
PWM al 75 %   ███▁███▁███▁
```

## El código, parte por parte

### Parte 1: pines, canal PWM y parámetros

**En palabras simples:** anotamos dónde está el flash, preparamos la herramienta del chip que
permite darle **brillo intermedio** (no solo prendido o apagado) y definimos qué se puede
ajustar desde el panel.

**Conceptos nuevos**

- **PWM (modulación por ancho de pulso):** un pin solo sabe estar prendido o apagado. Para
  lograr "medio brillo", se prende y apaga **muy rápido**, miles de veces por segundo, y el ojo
  ve el promedio. Si está prendido la mitad del tiempo, se ve a media luz. Es como un
  ventilador de techo que gira tan rápido que no ves las aspas.
- **Frecuencia (Hz):** cuántas veces por segundo se repite ese prende-apaga. 5000 Hz = 5000
  veces por segundo: demasiado rápido para notar parpadeo.
- **Resolución (bits):** cuántos niveles distintos puede tener el PWM. Con 12 bits hay 4096
  niveles, de 0 (apagado) a 4095 (siempre prendido). Más niveles permiten brillos muy bajos
  finos, que es donde el ojo más nota la diferencia (ver Parte 2).
- **Canal LEDC:** el chip tiene 16 "generadores de PWM" llamados canales. Hay que elegir uno y
  conectarle el pin. **Usamos el 4 porque la cámara usa el 0** para su propio reloj: si
  compartieran canal, tocar el brillo arruinaría las fotos.
- **`bool`:** tipo de dato que solo puede valer `true` (verdadero) o `false` (falso). Perfecto
  para recordar un sí/no, como "¿está respirando?".

**Línea por línea**

| Código | Qué hace |
|---|---|
| `const int LED_FLASH = 4;` | El flash está en el pin 4 (como en la lección 01). |
| `const int CANAL_PWM = 4;` | Usaremos el canal 4 de PWM. |
| `const int FRECUENCIA_HZ = 5000;` | 5000 prende-apaga por segundo. |
| `const int RESOLUCION_BITS = 12;` | 4096 niveles: de 0 a 4095. |
| `const int PWM_MAXIMO = 4095;` | El valor de PWM más alto: siempre prendido. |
| `BRILLO_MAXIMO` (parámetro, 50 %) | El brillo más alto permitido, en **porcentaje de lo que ve el ojo**. El flash es muy potente: al máximo encandila, se calienta y puede reiniciar la placa por falta de energía. |
| `PASO_MS` (parámetro, 15) | Cuántos milisegundos dura cada escalón del efecto respirar. Más alto = respiración más lenta. |
| `bool respirando = true;` | Recuerda qué está haciendo el flash: `true` = respira, `false` = brillo fijo. |

> OJO: el pin 4 también lo usa la tarjeta microSD. Si algún día usás una, el flash va a
> parpadear solo cuando se lea la tarjeta.

### Parte 2: funciones propias y la corrección gamma

**En palabras simples:** creamos tres **herramientas** con nombre: una que traduce "cuánto
brillo quiero ver" a "cuánto PWM mandar", otra para dejar el flash en un brillo fijo y otra para
el efecto de respirar. Así el resto del programa solo dice "respirá" sin repetir todo.

**Conceptos nuevos**

- **Función propia:** igual que `setup()` y `loop()`, pero la inventamos nosotros. Se escribe
  una vez y se usa ("se llama") todas las veces que haga falta.
- **Argumento y resultado:** el argumento es el dato que se le pasa entre paréntesis
  (`ponerBrillo(30)` recibe 30). Algunas funciones además **devuelven** un resultado con
  `return`: `porcentajeAPwm(50)` devuelve 891.
- **El ojo no ve la luz en proporción:** nota muchísimo los cambios cuando hay poca luz y casi
  nada cuando hay mucha. Lo medimos con la cámara de esta misma placa: con apenas 2 % de PWM la
  escena ya tiene el doble de luz, y de 25 % para arriba todo parece "prendido a fondo". Por eso,
  si repartiéramos el PWM en partes iguales, el flash pasaría casi todo el tiempo viéndose igual.
- **Corrección gamma:** la fórmula que compensa eso. En vez de mandar el porcentaje tal cual, se
  eleva a la potencia 2.2: `PWM = 4095 × (porcentaje / 100)^2.2`. Pedir 50 % manda 891 (el 22 %
  del PWM), que el ojo percibe como "la mitad". Es el mismo truco que usan las pantallas.
- **`pow(base, exponente)`:** calcula una potencia. `pow(0.5, 2.2)` = 0.5 elevado a 2.2.
- **`float`:** tipo de dato para números **con decimales** (0.5, 2.2). `int` no los admite.
- **`constrain(valor, mínimo, máximo)`:** recorta un número para que no se salga del rango. Si
  alguien pide 999 %, queda en el máximo permitido. **Nunca hay que confiar ciegamente en lo que
  escribe el usuario.**
- **Bucle `for`:** repite un bloque contando. `for (int p = 0; p <= 50; p++)` significa "empezá
  con `p` en 0; mientras `p` sea menor o igual a 50, repetí; y en cada vuelta sumale 1 a `p`".
- **`ledcWrite(canal, valor)`:** fija el PWM de un canal (de 0 a 4095).

**Línea por línea**

| Código | Qué hace |
|---|---|
| `int porcentajeAPwm(int porcentaje) {` | Función que recibe un porcentaje y **devuelve** un número entero (`int`). |
| `float fraccion = porcentaje / 100.0;` | Pasa de porcentaje a fracción: 50 → 0.5. El `.0` obliga a calcular con decimales. |
| `return round(PWM_MAXIMO * pow(fraccion, 2.2));` | Aplica la corrección gamma, redondea y devuelve el resultado. |
| `void ponerBrillo(int pedido) {` | Función que deja el flash fijo en el porcentaje `pedido`. |
| `int porcentaje = constrain(pedido, 0, BRILLO_MAXIMO);` | Lo recorta entre 0 y el tope. |
| `ledcWrite(CANAL_PWM, pwm);` | Aplica el PWM calculado. |
| `Serial.printf("Brillo: %d%% (PWM %d/4095)...")` | Muestra el porcentaje y el PWM real. `%%` escribe el símbolo %. |
| `if (porcentaje != pedido) { ... }` | Si hubo que recortar, avisa: "pediste 100 %, pero el tope es 50 %". Un programa que corrige en silencio confunde a quien lo usa. |
| primer `for` (p sube de 0 al máximo) | Enciende poco a poco, un porcentaje por paso, esperando `PASO_MS` entre cada uno. |
| segundo `for` (p baja con `p--`) | Apaga poco a poco. `p--` resta 1 en cada vuelta. |

Estas funciones **no tienen marca de parte propia**: se ejecutan cuando otra parte las llama.
Por eso, mientras el flash respira, el panel resalta la **Parte 4**, que es la que las está usando.

### Parte 3: `setup()`, conectar el flash al PWM

**En palabras simples:** al arrancar, configuramos el canal de PWM y le conectamos el pin del
flash. Desde ese momento el brillo se controla con `ledcWrite`.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `Serial.begin(921600);` | Abre el monitor serie (como en la lección 01). |
| `panel::iniciar("02_flash_pwm");` | Le dice al panel qué lección corre. |
| `panel::parte(3);` | Marca el inicio de la Parte 3. |
| `ledcSetup(CANAL_PWM, FRECUENCIA_HZ, RESOLUCION_BITS);` | Configura el canal 4: 5000 Hz y 4096 niveles. |
| `ledcAttachPin(LED_FLASH, CANAL_PWM);` | Conecta el pin del flash a ese canal. |

> Detalle técnico: esta es la forma de escribirlo en la versión 2 del "core" de Arduino para
> ESP32, la que fija el `platformio.ini`. En la versión 3 cambió a `ledcAttach(...)`. Por eso
> el curso fija la versión: el mismo código tiene que compilar igual para todos.

### Parte 4: `loop()`, respirar o esperar órdenes

**En palabras simples:** en cada vuelta, la placa se fija si le escribiste algo. Si escribiste
un número, deja el flash fijo en ese brillo; si escribiste `r`, vuelve a respirar. Si está en
modo respirar, hace un ciclo completo y vuelve a empezar.

**Conceptos nuevos**

- **`if` / `else` (si / si no):** permite decidir. "**Si** pasa esto, hacé A; **si no**, hacé B".
- **`String`:** tipo de dato para guardar texto.
- **`==`:** compara si dos cosas son iguales. No confundir con `=`, que **guarda** un valor.
- **`toInt()`:** convierte un texto como `"30"` en el número 30.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `panel::parte(4);` | Marca la Parte 4 (y si pediste pausa, se detiene acá). |
| `String linea;` | Crea una cajita para texto. |
| `if (panel::leerLinea(linea)) {` | **Si** escribiste algo, lo guarda en `linea` y entra al bloque. |
| `if (linea == "r") { respirando = true; }` | Si escribiste `r`, vuelve al modo respirar. |
| `else { respirando = false; ponerBrillo(...); }` | Si no, entiende que es un porcentaje: deja de respirar y fija ese brillo. |
| `if (respirando) respirar();` | Si está en modo respirar, hace un ciclo completo. |

**Probalo en el panel**

1. Mientras respira, cambiá `PASO_MS` a 40 en **Variables**: la respiración se vuelve lenta en
   el ciclo siguiente.
2. Bajá `BRILLO_MAXIMO` a 15: la respiración llega a un brillo mucho más suave.
3. Escribí `5`, después `20`, después `50` en el cuadro de envío: tres brillos fijos claramente
   distintos. Con `r` vuelve a respirar.

## Qué deberías ver

El flash respira sin parar. Escribís `10` + Enter y queda fijo y tenue; `30`, más fuerte; `0`
lo apaga; `100` queda en 50 % (el tope de fábrica de `BRILLO_MAXIMO`, y el monitor te lo avisa);
`r` vuelve a respirar. En el monitor:

```
Brillo: 10% (PWM 26/4095)
Brillo: 30% (PWM 290/4095)
Brillo: 50% (PWM 891/4095)
  Pediste 100%, pero el tope es BRILLO_MAXIMO = 50%. Subilo en Variables.
```

> Para llegar a 100 %, subí primero `BRILLO_MAXIMO` en **Variables**. Ojo: a fondo el flash
> se calienta rápido; no lo dejes así mucho tiempo.

> En el monitor de PlatformIO, para escribir: `Ctrl+T` y luego `Ctrl+E` activa el eco local
> y así ves lo que tecleás.

## Ejercicios

1. Agregá el comando `m` que ponga el brillo al máximo permitido.
2. Hacé que el LED rojo (GPIO 33, lógica invertida, en la cara de abajo de la placa) se encienda
   cuando el brillo supere 30 %.
3. Cambiá el 2.2 de la corrección gamma por 1 (sin corrección) y compará cómo respira. ¿Qué pasa
   con la mitad de arriba del ciclo?
4. Cambiá la resolución a 8 bits (`PWM_MAXIMO` = 255). ¿Qué pasa con los brillos 1 % a 5 %?
