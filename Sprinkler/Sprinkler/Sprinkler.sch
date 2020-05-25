EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L MCU_Module:WeMos_D1_mini U1
U 1 1 5EC469FC
P 3250 2250
F 0 "U1" H 3250 1361 50  0000 C CNN
F 1 "WeMos_D1_mini" H 3250 1270 50  0000 C CNN
F 2 "Module:WEMOS_D1_mini_light" H 3250 1100 50  0001 C CNN
F 3 "https://wiki.wemos.cc/products:d1:d1_mini#documentation" H 1400 1100 50  0001 C CNN
	1    3250 2250
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x04_Female J1
U 1 1 5EC79029
P 5200 2100
F 0 "J1" V 5138 1812 50  0000 R CNN
F 1 "BME280" V 5047 1812 50  0000 R CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 5200 2100 50  0001 C CNN
F 3 "~" H 5200 2100 50  0001 C CNN
	1    5200 2100
	0    -1   -1   0   
$EndComp
$Comp
L Connector:Conn_01x04_Female J2
U 1 1 5EC795CB
P 5300 2900
F 0 "J2" V 5146 3048 50  0000 L CNN
F 1 "OLED" V 5237 3048 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 5300 2900 50  0001 C CNN
F 3 "~" H 5300 2900 50  0001 C CNN
	1    5300 2900
	0    1    1    0   
$EndComp
Wire Wire Line
	5100 2300 5100 2700
Text Label 5100 2450 0    50   ~ 0
VCC
Text Label 5200 2550 0    50   ~ 0
GND
Wire Wire Line
	5300 2300 5300 2700
Text Label 5300 2600 0    50   ~ 0
SCL
Text Label 5400 2650 0    50   ~ 0
SDA
Wire Wire Line
	5100 2300 4500 2300
Wire Wire Line
	4500 2300 4500 1250
Wire Wire Line
	4500 1250 3350 1250
Wire Wire Line
	3350 1250 3350 1450
Connection ~ 5100 2300
Wire Wire Line
	3650 1950 4400 1950
Wire Wire Line
	4400 1950 4400 3100
Wire Wire Line
	4400 3100 5300 3100
Wire Wire Line
	5300 3100 5300 2700
Connection ~ 5300 2700
Wire Wire Line
	3650 2050 4350 2050
Wire Wire Line
	4350 2050 4350 3200
Wire Wire Line
	4350 3200 5400 3200
Wire Wire Line
	5400 2300 5400 2700
Connection ~ 5400 2700
Wire Wire Line
	5400 2700 5400 3200
$Comp
L power:GND #PWR0101
U 1 1 5EC7C681
P 4750 2900
F 0 "#PWR0101" H 4750 2650 50  0001 C CNN
F 1 "GND" V 4755 2772 50  0000 R CNN
F 2 "" H 4750 2900 50  0001 C CNN
F 3 "" H 4750 2900 50  0001 C CNN
	1    4750 2900
	0    1    1    0   
$EndComp
Wire Wire Line
	4750 2900 5000 2900
Wire Wire Line
	5000 2900 5000 3000
Wire Wire Line
	5000 3000 5200 3000
Wire Wire Line
	5200 2300 5200 2700
Connection ~ 5200 2700
Wire Wire Line
	5200 2700 5200 3000
$Comp
L Connector:Conn_01x02_Female J3
U 1 1 5EC80291
P 4000 3500
F 0 "J3" V 3846 3548 50  0000 L CNN
F 1 "Relay_Connector" V 3937 3548 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4000 3500 50  0001 C CNN
F 3 "~" H 4000 3500 50  0001 C CNN
	1    4000 3500
	0    1    1    0   
$EndComp
Wire Wire Line
	3650 2350 3900 2350
Wire Wire Line
	3900 2350 3900 3300
$Comp
L power:GND #PWR0102
U 1 1 5EC80FFC
P 4000 3150
F 0 "#PWR0102" H 4000 2900 50  0001 C CNN
F 1 "GND" H 4005 2977 50  0000 C CNN
F 2 "" H 4000 3150 50  0001 C CNN
F 3 "" H 4000 3150 50  0001 C CNN
	1    4000 3150
	-1   0    0    1   
$EndComp
Wire Wire Line
	4000 3300 4000 3150
$Comp
L power:GND #PWR?
U 1 1 5EC83171
P 3250 3350
F 0 "#PWR?" H 3250 3100 50  0001 C CNN
F 1 "GND" V 3255 3222 50  0000 R CNN
F 2 "" H 3250 3350 50  0001 C CNN
F 3 "" H 3250 3350 50  0001 C CNN
	1    3250 3350
	1    0    0    -1  
$EndComp
Wire Wire Line
	3250 3050 3250 3350
$EndSCHEMATC
