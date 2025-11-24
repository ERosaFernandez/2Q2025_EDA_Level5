# 2Q2025_EDA_Level5
Ejecución en linux:
./edahttpd -h ../www/ -m index
./edahttpd -h ../www/ -m image
./mkindex -w ../www/
./mkindex -s ../www/special/

ejecutar mkindex como ./mkindex -help (o mkindex.exe -help) para toda la info

Ejecución en Windows:
./edahttpd.exe -h ../../../../www/

En el modo IMAGE, los títulos están normalizados (todas mínusculas, carácteres ASCII)

# Observaciones
Es necesario instalar ICU para ejecutar el programa:
- En Linux: vcpkg install ICU
- En Windows
