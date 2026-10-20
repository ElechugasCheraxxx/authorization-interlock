#!/bin/bash
# Script de verificación para el Bundle de producción de Interlock Assessment IDE

APP_PATH="build_xcode/bin/Release/InterlockIDE_Qt.app"

if [ ! -d "$APP_PATH" ]; then
    echo "Error: Bundle no encontrado en $APP_PATH"
    echo "Asegúrate de haber compilado el proyecto en modo Release con el generador de Xcode."
    exit 1
fi

echo "================================================================"
echo " VERIFICACIÓN DE BUNDLE DE PRODUCCIÓN (macOS)"
echo "================================================================"

echo -e "\n1. Verificando RPATHs del binario principal..."
otool -l "$APP_PATH/Contents/MacOS/InterlockIDE_Qt" | grep -A 2 LC_RPATH

echo -e "\n2. Verificando vinculación de Python..."
otool -L "$APP_PATH/Contents/MacOS/InterlockIDE_Qt" | grep -i python

echo -e "\n3. Verificando existencia de librerías críticas en Frameworks..."
for lib in libpython3.14.dylib libllama.0.dylib libggml.0.dylib; do
    if [ -f "$APP_PATH/Contents/Frameworks/$lib" ]; then
        echo "[OK] $lib encontrado."
    else
        echo "[FAIL] $lib NO ENCONTRADO."
    fi
done

echo -e "\n4. Verificando modelos GGUF..."
COUNT=$(ls -1 "$APP_PATH/Contents/Resources/backend/models/"*.gguf 2>/dev/null | wc -l)
echo "[INFO] Modelos encontrados: $COUNT"
if [ "$COUNT" -gt 0 ]; then
    echo "[OK] Modelos empaquetados correctamente."
else
    echo "[FAIL] No se encontraron modelos en Contents/Resources/backend/models/"
fi

echo -e "\n5. Verificando firma digital (Ad-hoc)..."
codesign -vvv --deep "$APP_PATH"

echo -e "\n6. Verificando dependencias de dylibs internas..."
otool -L "$APP_PATH/Contents/Frameworks/libllama.0.dylib" | grep "@rpath"

echo "================================================================"
echo " Verificación completada."
echo "================================================================"
