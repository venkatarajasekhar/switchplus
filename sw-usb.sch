v 20110115 2
C 46300 46500 1 0 0 lpc-usb.sym
{
T 46700 51400 5 10 1 1 0 0 1
value=LPC USB
T 47600 51400 5 10 1 1 0 0 1
refdes=U?
}
C 42400 49600 1 0 0 micro-usb.sym
{
T 42600 51117 5 10 1 1 0 0 1
value=Micro USB
T 42600 50900 5 10 1 1 0 0 1
refdes=CONN?
T 42600 50700 5 10 1 1 0 0 1
footprint=micro-ab-usb
}
N 45000 50900 45800 50900 4
N 45800 50900 45800 50500 4
N 45800 50500 46300 50500 4
N 45000 50600 45900 50600 4
N 45900 50600 45900 50800 4
N 45900 50800 46300 50800 4
N 45000 51200 46300 51200 4
N 46300 51200 46300 51100 4
N 45000 50300 45800 50300 4
N 45800 50300 45800 50200 4
N 45800 50200 46300 50200 4
N 42700 49700 44200 49700 4
C 44900 49700 1 0 0 gnd-1.sym
C 45300 51200 1 0 0 5V-plus-1.sym
C 45400 49600 1 0 0 resistor-1.sym
{
T 45700 50000 5 10 0 0 0 0 1
device=RESISTOR
T 45600 49900 5 10 1 1 0 0 1
refdes=R?
}
N 45400 49100 45400 49700 4
T 42500 49100 9 10 1 0 0 0 2
FIXME -
decouple/protect USB0_VBUS
N 45400 49100 46300 49100 4
N 46300 49400 45400 49400 4
C 45800 48800 1 0 0 gnd-1.sym
C 43400 48500 1 0 0 fuse-2.sym
{
T 43600 49050 5 10 0 0 0 0 1
device=FUSE
T 43600 48200 5 10 1 1 0 0 1
refdes=FB?
T 43600 49250 5 10 0 0 0 0 1
symversion=0.1
}
C 43100 47900 1 0 0 fuse-2.sym
{
T 43300 48450 5 10 0 0 0 0 1
device=FUSE
T 43100 47700 5 10 1 1 0 0 1
refdes=FB?
T 43300 48650 5 10 0 0 0 0 1
symversion=0.1
}
C 44200 47500 1 90 0 capacitor.sym
{
T 43500 47700 5 10 0 0 90 0 1
device=CAPACITOR
T 43700 47500 5 10 1 1 90 0 1
refdes=C?
T 43300 47700 5 10 0 0 90 0 1
symversion=0.1
}
C 44500 48100 1 90 0 capacitor.sym
{
T 43800 48300 5 10 0 0 90 0 1
device=CAPACITOR
T 44600 48600 5 10 1 1 180 0 1
refdes=C?
T 43600 48300 5 10 0 0 90 0 1
symversion=0.1
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
T 45100 47600 5 10 1 1 0 6 1
refdes=LED?
T 44600 47000 5 10 0 0 270 2 1
symversion=0.1
}
C 46000 47500 1 90 1 led-1.sym
{
T 45400 46700 5 10 0 0 270 2 1
device=LED
T 46300 47300 5 10 1 1 0 6 1
refdes=LED?
T 45200 46700 5 10 0 0 270 2 1
symversion=0.1
}
C 44300 46800 1 0 0 resistor-1.sym
{
T 44600 47200 5 10 0 0 0 0 1
device=RESISTOR
T 44500 47100 5 10 1 1 0 0 1
refdes=R?
}
C 44900 46500 1 0 0 resistor-1.sym
{
T 45200 46900 5 10 0 0 0 0 1
device=RESISTOR
T 44900 46400 5 10 1 1 0 0 1
refdes=R?
}
N 45800 47500 46300 47500 4
N 45200 47800 46300 47800 4
N 44900 46600 44300 46600 4