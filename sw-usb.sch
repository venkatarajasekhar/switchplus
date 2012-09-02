v 20110115 2
C 46300 46500 1 0 0 lpc-usb.sym
{
T 48100 51100 5 10 1 1 0 0 1
value=LPC USB
T 48600 50900 5 10 1 1 0 0 1
refdes=U1
}
C 42600 49600 1 0 0 micro-usb.sym
{
T 42800 51117 5 10 1 1 0 0 1
value=Micro USB
T 42800 50900 5 10 1 1 0 0 1
refdes=CONN5
T 42800 50700 5 10 1 1 0 0 1
footprint=micro-ab-usb
}
N 45200 50900 45800 50900 4
N 45800 50900 45800 50500 4
N 45800 50500 46300 50500 4
N 45200 50600 46100 50600 4
N 46100 50600 46100 50800 4
N 46100 50800 46300 50800 4
N 45200 51200 45400 51200 4
N 46300 51600 46300 51100 4
N 45200 50300 45800 50300 4
N 45800 50300 45800 50200 4
N 45800 50200 46300 50200 4
N 42900 49700 44400 49700 4
C 45100 49700 1 0 0 gnd-1.sym
C 45400 49600 1 0 0 resistor-1.sym
{
T 45700 50000 5 10 0 0 0 0 1
device=RESISTOR
T 45700 49850 5 10 1 1 0 0 1
refdes=R60
T 45400 49600 5 10 0 1 0 0 1
footprint=0603
}
N 45400 49100 45400 49700 4
N 45400 49100 46300 49100 4
N 46300 49400 45400 49400 4
C 45800 48800 1 0 0 gnd-1.sym
C 43400 48500 1 0 0 fuse-2.sym
{
T 43600 49050 5 10 0 0 0 0 1
device=FUSE
T 43700 48300 5 10 1 1 0 0 1
refdes=FB5
T 43600 49250 5 10 0 0 0 0 1
symversion=0.1
T 43400 48500 5 10 0 1 0 0 1
footprint=0603
}
C 43100 47900 1 0 0 fuse-2.sym
{
T 43300 48450 5 10 0 0 0 0 1
device=FUSE
T 43100 47700 5 10 1 1 0 0 1
refdes=FB6
T 43300 48650 5 10 0 0 0 0 1
symversion=0.1
T 43100 47900 5 10 0 1 0 0 1
footprint=0603
}
C 44200 47500 1 90 0 capacitor.sym
{
T 43500 47700 5 10 0 0 90 0 1
device=CAPACITOR
T 43600 47500 5 10 1 1 0 0 1
refdes=C81
T 43300 47700 5 10 0 0 90 0 1
symversion=0.1
T 44400 47700 5 10 1 1 0 0 1
value=4u7
T 44200 47500 5 10 0 1 0 0 1
footprint=0402
}
C 44500 48100 1 90 0 capacitor.sym
{
T 43800 48300 5 10 0 0 90 0 1
device=CAPACITOR
T 44650 48550 5 10 1 1 180 0 1
refdes=C82
T 43600 48300 5 10 0 0 90 0 1
symversion=0.1
T 44400 48100 5 10 1 1 0 0 1
value=4u7
T 44500 48100 5 10 0 1 0 0 1
footprint=0402
}
N 44300 46600 44300 48100 4
N 44300 47500 44000 47500 4
N 43100 48000 43100 48600 4
N 43100 48600 43400 48600 4
C 42500 48000 1 0 0 3.3V-plus-1.sym
N 42700 48000 43100 48000 4
C 43900 47200 1 0 0 gnd-1.sym
N 44300 48600 46300 48600 4
N 44000 48000 44800 48000 4
N 44800 48000 44800 48300 4
N 44800 48300 46300 48300 4
C 45400 47800 1 90 1 led-1.sym
{
T 44800 47000 5 10 0 0 270 2 1
device=LED
T 45000 47500 5 10 1 1 0 6 1
refdes=D12
T 44600 47000 5 10 0 0 270 2 1
symversion=0.1
T 45400 47800 5 10 0 1 0 0 1
footprint=s0805
}
C 46000 47500 1 90 1 led-1.sym
{
T 45400 46700 5 10 0 0 270 2 1
device=LED
T 46200 47300 5 10 1 1 0 6 1
refdes=D13
T 45200 46700 5 10 0 0 270 2 1
symversion=0.1
T 46000 47500 5 10 0 1 0 0 1
footprint=s0805
}
C 44300 46800 1 0 0 resistor-1.sym
{
T 44600 47200 5 10 0 0 0 0 1
device=RESISTOR
T 44400 47050 5 10 1 1 0 0 1
refdes=R61
T 44300 46800 5 10 0 1 0 0 1
footprint=0603
}
C 44300 46500 1 0 0 resistor-1.sym
{
T 44600 46900 5 10 0 0 0 0 1
device=RESISTOR
T 45100 46650 5 10 1 1 0 0 1
refdes=R62
T 44300 46500 5 10 0 1 0 0 1
footprint=0603
}
N 45800 47500 46300 47500 4
N 45200 47800 46300 47800 4
T 42800 48800 9 20 1 0 0 0 1
13. USB
C 45400 51500 1 0 1 passive-1.sym
{
T 44500 51700 5 10 0 0 0 6 1
net=USB5V:1
T 45200 52200 5 10 0 0 0 6 1
device=none
T 45200 51600 5 10 1 1 0 7 1
value=USB5V
}
C 45400 51500 1 0 0 resistor-1.sym
{
T 45700 51900 5 10 0 0 0 0 1
device=RESISTOR
T 45600 51300 5 10 1 1 0 0 1
refdes=R58
T 45900 51600 5 10 0 1 0 0 1
footprint=0603
}
N 45200 46600 45800 46600 4
N 45400 51200 45400 51600 4
C 46300 51500 1 0 0 resistor-1.sym
{
T 46600 51900 5 10 0 0 0 0 1
device=RESISTOR
T 47200 51550 5 10 1 1 0 0 1
refdes=R59
T 46600 51600 5 10 0 1 0 0 1
footprint=0603
}
C 47100 51300 1 0 0 gnd-1.sym
