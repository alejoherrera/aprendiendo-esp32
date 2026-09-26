/**
 * Conexion con Gemini (Google): le manda una foto y una pregunta, y devuelve la respuesta.
 *
 * Usa la API generateContent por HTTPS verificando el certificado de Google. La foto se
 * codifica en base64 de a pedazos mientras se envia, sin reservar memoria para toda la foto.
 */
#pragma once

#include <Arduino.h>

/**
 * Pregunta a Gemini sobre una foto JPEG.
 * @param modelo    codigo del modelo, ej. "gemini-3.5-flash-lite"
 * @param claveApi  la API key del estudiante (nunca se imprime)
 * @param respuesta si sale bien: el texto de Gemini; si sale mal: un mensaje para el estudiante
 * @return true si Gemini respondio
 */
bool preguntarAGemini(const uint8_t* jpeg, size_t largo, const String& pregunta,
                      const char* modelo, const String& claveApi, String& respuesta);
