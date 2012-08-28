v 20110115 2
C 39300 44800 1 0 0 spartan6-qfp144-bank2.sym
{
T 41100 48500 5 10 1 1 0 0 1
refdes=U3
T 40000 45000 5 10 1 1 0 0 1
device=Spartan6 Bank 2
T 41900 45000 5 10 1 1 0 0 1
footprint=qfp144
}
C 45100 46500 1 0 0 at45db081.sym
{
T 45900 47700 5 10 1 1 0 0 1
device=AT45D081
T 46200 47400 5 10 1 1 0 0 1
refdes=U5
T 46700 47100 5 10 1 1 0 0 1
footprint=so8
}
N 44200 47700 45100 47700 4
N 40200 49800 47500 49800 4
N 47500 48000 47500 49800 4
N 39300 49800 39300 49200 4
N 39300 49200 39500 49200 4
N 43300 49200 44500 49200 4
N 44500 47400 45100 47400 4
N 47700 48600 47700 49400 4
N 47700 49400 44800 49400 4
N 44800 49400 44800 48000 4
N 44800 48000 43300 48000 4
C 43300 46700 1 0 0 resistor-1.sym
{
T 43600 47100 5 10 0 0 0 0 1
device=RESISTOR
T 43600 46500 5 10 1 1 0 0 1
refdes=R9
T 43300 46700 5 10 0 1 0 0 1
footprint=0603
T 43300 46700 5 10 0 1 0 0 1
value=PULL
}
C 43300 48800 1 0 0 resistor-1.sym
{
T 43600 49200 5 10 0 0 0 0 1
device=RESISTOR
T 43600 48600 5 10 1 1 0 0 1
refdes=N1
T 43300 48800 5 10 0 1 0 0 1
footprint=0603
T 43300 48800 5 10 0 1 0 0 1
value=DNP
}
C 44100 48600 1 0 0 gnd-1.sym
C 44100 46500 1 0 0 gnd-1.sym
N 46300 48600 46300 48900 4
N 45000 48900 46600 48900 4
N 45000 48900 45000 47000 4
N 45000 47100 45100 47100 4
N 45100 48000 45000 48000 4
C 46200 46200 1 0 0 gnd-1.sym
C 45200 46500 1 90 0 capacitor.sym
{
T 44500 46700 5 10 0 0 90 0 1
device=CAPACITOR
T 44900 47000 5 10 1 1 180 0 1
refdes=C23
T 44300 46700 5 10 0 0 90 0 1
symversion=0.1
T 45900 46700 5 10 0 1 0 0 1
footprint=0603
T 45100 46800 5 10 0 1 0 0 1
value=100n
}
N 45000 46500 46300 46500 4
C 38600 48800 1 0 0 resistor-1.sym
{
T 38900 49200 5 10 0 0 0 0 1
device=RESISTOR
T 38900 49000 5 10 1 1 0 0 1
refdes=R5
T 38600 48800 5 10 0 1 0 0 1
footprint=0603
T 38600 48800 5 10 0 1 0 0 1
value=PULL
}
C 44400 49200 1 270 0 resistor-1.sym
{
T 44800 48900 5 10 0 0 270 0 1
device=RESISTOR
T 44200 48400 5 10 1 1 270 0 1
refdes=R7
T 44400 49200 5 10 0 1 270 0 1
footprint=0603
T 44400 49200 5 10 0 1 0 0 1
value=STERM
}
C 43300 47600 1 0 0 resistor-1.sym
{
T 43600 48000 5 10 0 0 0 0 1
device=RESISTOR
T 43700 47400 5 10 1 1 0 0 1
refdes=R8
T 43300 47600 5 10 0 1 0 0 1
footprint=0603
T 43800 47600 5 10 0 1 0 0 1
value=STERM
}
C 47800 47700 1 90 0 resistor-1.sym
{
T 47400 48000 5 10 0 0 90 0 1
device=RESISTOR
T 48000 47900 5 10 1 1 90 0 1
refdes=R11
T 47800 47700 5 10 0 1 90 0 1
footprint=0603
T 47800 47700 5 10 0 1 90 0 1
value=STERM
}
N 44500 48300 44500 47400 4
C 39300 49700 1 0 0 resistor-1.sym
{
T 39600 50100 5 10 0 0 0 0 1
device=RESISTOR
T 40100 49600 5 10 1 1 0 0 1
refdes=R6
T 40000 49800 5 10 0 1 0 0 1
footprint=0603
T 40000 49700 5 10 0 1 0 0 1
value=STERM
}
C 46600 48800 1 0 0 resistor-1.sym
{
T 46900 49200 5 10 0 0 0 0 1
device=RESISTOR
T 47100 49100 5 10 1 1 0 0 1
refdes=R10
}
N 47700 47700 47500 47700 4
C 38400 48900 1 0 0 3.3V-plus-1.sym
C 46100 48900 1 0 0 3.3V-plus-1.sym
T 44200 45500 9 20 1 0 0 0 1
3. FPGA Bank 2
C 39500 48500 1 0 1 io-1.sym
{
T 38600 48700 5 10 0 0 0 6 1
net=VD7:1
T 39300 49100 5 10 0 0 0 6 1
device=none
T 39200 48600 5 10 1 1 0 7 1
value=VD7
}
C 39500 48200 1 0 1 io-1.sym
{
T 38600 48400 5 10 0 0 0 6 1
net=VD13:1
T 39300 48800 5 10 0 0 0 6 1
device=none
T 39200 48300 5 10 1 1 0 7 1
value=VD13
}
C 39500 47300 1 0 1 io-1.sym
{
T 38600 47500 5 10 0 0 0 6 1
net=LCDLE:1
T 39300 47900 5 10 0 0 0 6 1
device=none
T 39300 47400 5 10 1 1 0 7 1
value=LCDLE
}
C 39500 47600 1 0 1 io-1.sym
{
T 38600 47800 5 10 0 0 0 6 1
net=VD18:1
T 39300 48200 5 10 0 0 0 6 1
device=none
T 39200 47700 5 10 1 1 0 7 1
value=VD18
}
C 39500 47900 1 0 1 io-1.sym
{
T 38600 48100 5 10 0 0 0 6 1
net=VD23:1
T 39300 48500 5 10 0 0 0 6 1
device=none
T 39200 48000 5 10 1 1 0 7 1
value=VD23
}
