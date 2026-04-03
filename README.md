# Proyecto #1 - Arreglos Paginados
**CE 1103 - Algoritmos y Estructuras de Datos II**
Instituto Tecnológico de Costa Rica — I Semestre 2026
Autor: Isaac Arias Picado

## Requisitos
- Windows 10/11 x64
- Visual Studio 2022 con soporte C++17

## Compilación
1. Abrir `Sorter.sln` en Visual Studio
2. Seleccionar **Release x64**
3. Build → Build Solution

## Uso

### Generador
```powershell
.\generator.exe -size <SIZE> -output <RUTA>
```
`<SIZE>`: `SMALL` (256 MB), `MEDIUM` (512 MB), `LARGE` (1 GB)
```powershell
.\generator.exe -size SMALL -output datos.bin
```

### Ordenador
```powershell
.\sorter.exe -input <ENTRADA> -output <SALIDA> -alg <ALG> -pageSize <BYTES> -pageCount <N>
```

| `-alg` | Algoritmo |
|--------|-----------|
| `QS` | Quick Sort |
| `MS` | Merge Sort |
| `TS` | Tim Sort |
| `RS` | Radix Sort |
| `SS` | Shell Sort |
```powershell
.\sorter.exe -input datos.bin -output ordenado -alg RS -pageSize 16384 -pageCount 512
```

Genera `ordenado.bin` (binario) y `ordenado.txt` (legible con comas).

## Repositorio
https://github.com/Isaac-Arias-Picado/Proyecto-Datos-2-Generator-Sorted.git
