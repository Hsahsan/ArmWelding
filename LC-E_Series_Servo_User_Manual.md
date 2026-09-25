# LC-E Series AC Servo Drive
## EtherCAT Bus Servo User Manual

**Source version:** V3.12

> Markdown conversion from the supplied PDF. Figures and diagrams are represented by nearby extracted captions/text; verify wiring and dimensions against the original PDF.


---

<!-- PDF page 1 -->

LC-E Series AC Servo Drive
EtherCAT Bus Servo User Manual




           Shenzhen Xinlichuan Electric Co., Ltd.


---

<!-- PDF page 3 -->

版本号：V3.12


                                                                           Contents
Chapter I Safety Precautions .............................................................................................................................................. 1
Chapter II Electrical Specifications ................................................................................................................................... 1
   2.1 Specifications .......................................................................................................................................................... 1
   2.2 Drive model ............................................................................................................................................................ 2
Chapter III Installation ....................................................................................................................................................... 3
   3.1 Installation of servo drive unit .............................................................................................................................. 3
         3.1.1 Installation environment ............................................................................................................................... 3
         3.1.2 Installation method........................................................................................................................................ 3
         3.1.3 Installation dimensions ................................................................................................................................. 4
   3.2 Servo motor installation ........................................................................................................................................ 4
         3.2.1 Installation environment ............................................................................................................................... 4
         3.2.2 Installation method........................................................................................................................................ 4
Chapter IV Wiring ............................................................................................................................................................... 5
   4.1 Terminal description ............................................................................................................................................ 5
   4.1 Main circuit wiring ............................................................................................................................................... 6
         4.2.1 Definition of main circuit terminals .............................................................................................................. 6
         4.2.2 How to use the main circuit power terminal (spring type) ............................................................................ 6
         4.2.3 Drive wiring diagram .................................................................................................................................... 7
   4.3 Definition of wiring terminal ................................................................................................................................ 8
         4.3.3 Encoder terminal definition（CN3） ........................................................................................................... 8
         4.3.4 RS485 Terminal Definition（CN4/CN5） ................................................................................................... 9
   4.4 Control signal terminal wiring.............................................................................................................................. 9
         4.4.1 DI input circuit .............................................................................................................................................. 9
         4.4.2 DO output circuit ........................................................................................................................................ 10
   4.5 Detailed description of DI/DO port function configuration ............................................................................. 10
         4.5.1 DI function description ............................................................................................................................... 10
         4.5.2 DO function description .............................................................................................................................. 11
Chapter V Panel Display and Operation ......................................................................................................................... 12
   5.1 Panel introduction and description ................................................................................................................... 12
         5.1.1 Description of panel keys ............................................................................................................................ 12
         5.1.2 How to operate keys on the panel ............................................................................................................... 12
         5.1.3 Status display .............................................................................................................................................. 12
         5.1.4 Status display .............................................................................................................................................. 13
         5.1.5 Parameter value display .............................................................................................................................. 13
         5.1.6 Monitoring parameter display (P0B group parameters) .............................................................................. 14
   5.2 Common operations ............................................................................................................................................. 15
         5.2.1 Initialization parameters.............................................................................................................................. 15
         5.2.2 Manual reset alarm ...................................................................................................................................... 15
         5.2.3 JOG mode operation ................................................................................................................................... 15
   5.3 Description of Gain Parameter Setting .............................................................................................................. 16
Chapter VI Communication configuration ..................................................................................................................... 17
   6.1 EtherCAT networking diagram .......................................................................................................................... 17
         6.1.1 EtherCAT communication technology specifications ................................................................................. 17
   6.2 Driver related parameter configuration ............................................................................................................ 18
         6.2.1 system parameter setting ............................................................................................................................. 18
         6.2.2 Rotation direction selection ........................................................................................................................ 18
         6.2.3 Holding brake setting .................................................................................................................................. 18
   6.3 Communication cycle of each mode ................................................................................................................... 19
   6.4 Process Data PDO ................................................................................................................................................ 19


---

<!-- PDF page 4 -->

版本号：V3.12
         6.4.1 Variable PDO mapping ................................................................................................................................19
         6.4.2 Fixed PDO mapping ....................................................................................................................................19
Chapter VII Control mode description ............................................................................................................................21
   7.1 Introduction to Control ........................................................................................................................................21
         7.1.1 Control word 6040h.....................................................................................................................................22
         7.1.2 Status word 6041h .......................................................................................................................................22
   7.2 Working mode.......................................................................................................................................................23
         7.2.1 Introduction to servo mode..........................................................................................................................23
         7.2.2 Servo mode switching .................................................................................................................................23
   7.3 Periodic synchronous position mode (CSP mode) .............................................................................................23
   7.4 Periodic synchronous speed mode (CSV mode) .................................................................................................24
   7.5 Periodic synchronous torque mode (CST mode) ...............................................................................................25
   7.6 Profile position mode (PP mode) .........................................................................................................................26
   7.7 Profile speed mode (PV mode) ............................................................................................................................27
   7.8 Contour torque mode (PT mode) ........................................................................................................................28
   7.9 Return to Origin Mode(Home mode) .................................................................................................................29
   7.10 Probe Function Description ...............................................................................................................................30
         7.10.1 Function Description of 60B8h and 60B9h ...............................................................................................31
         7.10.2 Read the probe latch position ....................................................................................................................32
Chapter VIII Parameter Description ...............................................................................................................................33
   8.1 1000h object dictionary ........................................................................................................................................33
   8.2 2000h Object Dictionary Parameters ..............................................................................................................34
         8.2.1 2001 Group object dictionary (P01 group parameter) ..............................................................................34
         8.2.2 2002 Group Object Dictionary (P02 Group Parameters) .........................................................................34
         8.2.3 2003 Group Object Dictionary (P03 Group Parameters) .........................................................................35
         8.2.4 2004 Group Object Dictionary (P04 Group Parameters) .........................................................................35
         8.2.5 2005 Group Object Dictionary (P05 Group Parameters) .........................................................................36
         8.2.6 2006 Group Object Dictionary (P06 Group Parameters) .........................................................................37
         8.2.7 2007 Group Object Dictionary (P07 Group torque control parameters) ..................................................37
         8.2.8 2008 Group Object Dictionary (P08 Group gain class parameters) .........................................................38
         8.2.9 2009 Group Object Dictionary (P09 Group self adjustment parameters) ................................................39
         8.2.10 200A Group Object Dictionary (P0A Group Fault and Protection Parameters) .....................................40
         8.2.11 P0B Group monitoring parameters .........................................................................................................41
         8.2.12 200C Group Object Dictionary (P0C Group communication parameters) .............................................41
         8.2.13 P0D Group auxiliary function parameters ..............................................................................................42
   8.3 6000 Group Object Dictionary .........................................................................................................................43
Chapter IX Troubleshooting..............................................................................................................................................46
   9.1 Fault and Warning Code List ..............................................................................................................................46
         9.1.1 Fault code table (to reset the fault, you need to cancel the enable first) ......................................................46
         9.1.2 Warning code table (warnings can be reset directly without canceling the enable) ....................................49
Appendix 1 Shutdown method ..........................................................................................................................................50
Appendix 2 Servo home mode ...........................................................................................................................................51


---

<!-- PDF page 5 -->

EtherCAT 总线伺服驱动器用户手册

                                   Chapter I Safety Precautions
Before using the servo drive system, please read the related precautions carefully, and be
sure to abide by the safety precautions and operating procedures for installation and
commissioning. The company is not responsible for any equipment damage or personal
injury caused by not operating in accordance with the requirements.
◆ This product is a general industrial product and is not intended for use in machines and
systems that are related to human life.
◆ Only qualified personnel are allowed to perform wiring, running, maintenance,
inspection and other operations.
◆ Safety devices must be equipped if it is used on devices that may cause serious accidents
or losses.

◆ Although this product is perfectly sound in terms of quality control, the noise, static

electricity, input power supply, wiring, parts and other factors may cause unexpected actions.

Please fully consider mechanical safety measures to ensure safety within the possible range

of motion.

                             Chapter II Electrical Specifications

2.1 Specifications

    Input power        Single-phase 220V

                       Temperature         0～45℃
                       Humidity            ≤90%RH, no condensation
    Working            Elevation           Altitude ≤1000m
    environment        Installation
                                           No corrosive gas, flammable gas, oil mist or dust.
                       environment
                       Installation mode   Vertical
                                           Support 17-bit incremental/absolute value encoder,
    Encoder
                                           23-bit incremental/absolute value encoder
    Output power    24V voltage output     100mA, supply power to DI port and pulse port.
                    Digital input          5-channel common digital input, function can be configured.
    Control signal
                    Digital output         3-channel digital output, function can be configured.
    Communication function                 EtherCAT communication.
    Display panel and key operation        5 keys (Mode, Set, Left, Up, Down) and 6 nixie tubes
                                           Built-in 50W 40Ω braking resistor.For frequent braking occasions, an
    Braking resistor
                                           external braking resistor is required.


                                                                                                                  -1-


---

<!-- PDF page 6 -->

EtherCAT 总线伺服驱动器用户手册

2.2 Drive model


                                LC - 10 E - 100
                                  ①       ② ③         ④


                     ① : Drive series               ④ : Motor power
                     ② : Driver power                   50:50w
                         10: 50W~750W                   100:100W
                         20: 1KW                        200:200W
                         30:1KW~2.6KW                   400:400W
                         50:3KW~3.8KW                   750:750W
                     ③ : Control type                   1000:1KW
                         P: Pulse type                  ……
                         E: EtherCAT type               3800:3.8KW



2.3 Motor model


             LCMT - 02             LB C17 N B - 60 M006 30B
               ①          ② ③ ④           ⑤   ⑥ ⑦          ⑧       ⑨       ⑩

            ① : Motor series                  ⑥ : Motor brake
            ② : Motor power                       N: Without brakes
                02:0.2KW                          Z: With brakes
                04:0.4KW                      ⑦ : Motor oil seal and keyway
                ……                                A: No oil seal, no keyway
                38:3.8KW                          B: With oil seal and keyway
            ③ : Number of motor poles             C: With oil seal, without keyway
                □:4 pairs of poles            ⑧ : Motor flange
                S:5 pairs of poles                60:60 flange
            ④：Motor inertia                       80:80 flange
                LB:220V low inertia               130:130 flange
                MB:220V Medium inertia        ⑨：motor torque
            ⑤：Encoder type                    ⑩：motor speed
                C17: 17 bit incremental           10:1000RPM
                     magnetic encoding            15:1500RPM
                R17: 17 bit magnetic              ……
                     encoder absolute value       30:3000RPM
                C23: 23 bit incremental
                     magnetic encoding
                R23: 23 bit magnetic
                     encoder absolute value




  -2-


---

<!-- PDF page 7 -->

EtherCAT 总线伺服驱动器用户手册

                                             Chapter III Installation

                                                               Warning
● The storage and installation of the product must meet the environmental conditions.
● Damaged or incomplete products should not be installed and used.
● The product requires fireproof materials for installation, and must not be installed on or near flammable
materials to prevent fire.
● The servo drive unit must be installed in the electric cabinet to prevent the intrusion of dust, corrosive gas,
      conductive objects, liquids, and inflammables.
● Servo drive unit and servo motor should be protected from vibration and shock.
● It is strictly forbidden to drag the servo motor wires and encoder lines.

3.1 Installation of servo drive unit

                                                                Note
● The servo drive unit must be installed in a well-protected electric cabinet.
● The servo drive unit must be installed in the specified direction and interval, and ensure good heat
    dissipation conditions.
● Do not install on or near flammable objects to prevent fire.

3.1.1 Installation environment
   Operating temperature/humidity: 0~55℃ (no frost), < 90%RH (no condensation).
   Storage temperature/humidity: -20~65℃ (no frost), < 90%RH (no condensation).
   Atmospheric environment: inside the control cabinet, without corrosive or flammable gas, oil mist, dust, etc.
   Elevation: below 1000m above sea level.
   Vibration: < 0.5G (4.9m/s2), 10~60 Hz (non-continuous operation).
   Protection: The servo drive itself has no protection, so it must be installed in a well-protected electrical cabinet, and
    protected from the intrusion of corrosive or flammable gases, conductive objects, metal dust, oil mist and liquids.

3.1.2 Installation method
   The servo drive of our company is in vertical structure, so please install it vertically. The installation direction should
    be upwards perpendicular to the installation surface.
   The installation layout of single or multiple servo drives is shown in the figure below.




          Installation interval of a single servo drive unit    Installation interval of multiple servo units
                                                                                                                            -3-


---

<!-- PDF page 8 -->

EtherCAT 总线伺服驱动器用户手册
3.1.3 Installation dimensions
                                                                       196,00

                 47,50                                        161,00                                                   39,72



                                                                                                                  R2
                                                                                                                    ,50
                                                                                                                       *3
    169,50




                                                                                                         161,00
                                                                                                                    25,00



                                              LC-10E/LC-20E Drive Dimensions

3.2 Servo motor installation

                                                                Warning
● It is strictly forbidden to knock the shaft end of the motor, or the motor encoder may be damaged.




3.2.1 Installation environment
      Operating temperature/humidity: 5~40℃ (no frost), < 90%RH (no condensation).
      Storage temperature/humidity: -20~55℃ (no frost), < 80%RH (no condensation).
      Atmospheric environment: indoor (no exposure), without corrosive or flammable gas, oil mist, dust, etc.
      Elevation: below 1000m above sea level.
      Vibration: < 0.5G (4.9m/s2), 10~60 Hz (non-continuous operation).
      Protection level: IP54
3.2.2 Installation method
      Installation direction: To prevent water, oil and other liquids from flowing into the motor from the motor outlet, please
       place the cable outlet at the bottom. If the motor shaft is installed upwards and a reducer is attached, it is necessary to
       prevent oil stains in the reducer from penetrating into the motor from the motor shaft.
      Concentric: When connecting with the machine, please use the coupling, and keep the axle center of the servo motor
       and the axle center of the machine in a straight line.
      Cable: Do not "bend" or put "tension" on the cables, do not over-tension the cable when wiring (using).
      Fixing: The motor installation must be firm, and there should be anti-loosening measures.
    -4-


---

<!-- PDF page 9 -->

EtherCAT 总线伺服驱动器用户手册

                                      Chapter IV Wiring

                                                       Warning
● The power supply of this series of drives is Single phase or three-phase 220V. The power supply must be
   identified when wiring.
● When using this product, the user must consider safety protection measures in the design and assembly to
   prevent accidents caused by wrong operations.
● The drive terminals U, V and W must correspond to the motor U, V and W, or it may cause a crash.
● The drive and motor must be well grounded.
● The power must be turned off at least 5 minutes before disassembling the drive.
● It is forbidden to turn on/off the power frequently. When the power is off, you need to wait for the nixie
   tube to go out before powering on again.
● When using the internal braking resistor, the short-circuit wire must be connected between terminals B2
   and B3. It is forbidden to connect the wire between B1 and B2 directly.


4.1 Terminal description

                                                                                A C 220V




                                                                     IN   OUT


                                    EtherCAT
                                                              C N1




                                Communication port
                                                              C N2




                                  IO signal terminal
                                                             CN3




                                 Encoder terminals

                                                                          L
                                           AC Power
                                                                          N
                                                                          B1
                              Brake resistor terminal                     B3
                                                                          B2
                                                                          U
                                                                          V
                         Motor power line terminals
                                                                          W
                                                                          PE
                               Ground wire terminal                       PE




                                            LC-10E/LC-20E driver

                                                                                                         -5-


---

<!-- PDF page 10 -->

EtherCAT 总线伺服驱动器用户手册

4.1       Main circuit wiring

4.2.1 Definition of main circuit terminals

     Input power terminal of LC-10/LC-20 driver

       No.     Signal definition                                       Functions
          1            L
                                             Power terminal, can be connected to AC single-phase 220V
          2            N

     制动电阻端子

       No.     Signal definition            Functions                                   Description
                                   DC bus positive terminal         The positive terminal of the built-in resistor is
          1            B1
                                   output DCP                       connected to B1. When using the built-in
                                   Built-in braking resistor        resistor, please short-circuit B2 and B3. When
          2            B3
                                   negative output.                 using external resistor, please connect the
                                   Brake transistor collector       resistor between B1 and B2 (B2 and B3 must be
          3            B2
                                   output                           disconnected).

     Motor terminal

       No.     Signal definition                                       Functions
          1            U                                        Connect to motor phase U
          2            V                                        Connect to motor phase V
          3            W                                        Connect to motor phase W
          4            PE                                       Connect to motor housing

4.2.2 How to use the main circuit power terminal (spring type)
1. Strip the outer sheath of the wire to expose 8-9mm bare copper wire.
2. The pressing method is as follows:
     ● Use the control bar provided with the servo drive to pry up the slot (as shown in Fig. A);
     ● Insert a slotted screwdriver into the terminal opening (3.0~3.5mm width at the end), then press firmly
to open the slot (as shown in Fig. B).
3. The pressing method is as follows:




                                   Fig. A                                      Fig. B




    -6-


---

<!-- PDF page 11 -->

EtherCAT 总线伺服驱动器用户手册
4.2.3 Drive wiring diagram

 LC-10E/LC-20E Driver Wiring Diagram

                                                    USB to 485 communication cable




                                                                     Servo driver


                                                                        IN     OUT
                              EtherCAT
                              controller


                       Limit, origin, and


                                                                       CN2
                        other IO signals

               Filters are selectively installed
               according to the on-site environment
                                                                       CN3


                        MCCB
                                                                        L
           AC 220V                       滤波器
                                                                       N
                                                                      B1
                       External braking
                           resistor                                   B3
                                                                      B2
                                                                                                        Motor

              ① When using internal braking
                                                                       U
              resistors, short-circuit B2 and B3;
              ② When using an external braking                         V
              resistor, it must be disconnected
                                                                       W
              B2 and B3, connect the resistors to B1
              and B2.                                                 PE
                                                                      PE

Note: When using the internal braking resistor, short-circuit B2 and B3 (connected at the factory); when using the external
      braking resistor, disconnect B2 and B3, and connect the external braking resistor between B1 and B2.




                                                                                                                        -7-


---

<!-- PDF page 12 -->

EtherCAT 总线伺服驱动器用户手册

4.3 Definition of wiring terminal

4.3.1 EtherCAT Terminal Definition（CN1）
    Pin             color          definition         Description
         1    White/orange            TX+       EtherCAT sending data+
                                                                                               Pin1                      Pin8
         2      Orange                TX-       EtherCAT sending data-
         3    White/green             RX+       EtherCAT receive data+
         4          Blue               /                     /
         5     White/blue              /                     /
         6         Green              RX-       EtherCAT receive data-
         7    White/brown              /                     /
         8       Brown                 /                     /
                                                                                             Registered jack pin order

4.3.2 Definition of control terminals（CN2）
             Signal                                                                                          Pin definition
    Pin                        Function name                  Notes or supplementary instructions
             name                                                                                                image
    11       DO0A       Digital output 0-A terminal       Detailed description of parameter
                                                          configuration, as described in Chapter 4.5.2.
    12       DO0B       Digital output 0-B terminal
    13       DO1A       Digital output 1-A terminal       Attention: The maximum allowable current
    14       DO1B       Digital output 1-B terminal       for the output port is 200mA, and it cannot
                                                          directly drive high current loads such as
    15       DO2A       Digital output 2-A terminal
                                                          motor brake. An external relay needs to be           15         16
    16       DO2B       Digital output 2-B terminal       added.
                                                                                                               13         14
     7        24V       DC 24V power output positive      The maximum current output of 24V is
     9        24V       terminal                          100mA, which can only be used as a DI port           11         12
                                                          and pulse signal power supply. It is                 9          10
     8        0V        DC 24V power output negative      prohibited to use it to drive external
    10        0V        terminal                          loads.                                               7          8
     6       DICO                                         DICOM can be connected to+24V or 0V,                 5          6
                        DI port common terminal           Please refer to Chapter 4.5.1 for instructions.
              M                                                                                                3          4
     5        DI0       Digital input 0                                                                        1           2
     4        DI1       Digital input 1
     3        DI2                                         Detailed description of parameter
                        Digital input 2
                                                          configuration, as described in Chapter 4.5.1.
     2        DI3       Digital input 3
     1        DI4       Digital input 4

4.3.3 Encoder terminal definition（CN3）
 Schematic diagram of drive encoder terminals

                                                                 ⑤   ③ ①


                                                                 ⑥   ④ ②


                                                       Encoder terminal pin diagram



   -8-


---

<!-- PDF page 13 -->

EtherCAT 总线伺服驱动器用户手册
 Drive encoder pin definition
                             Servo side                             Name               Wire color
                         1              VCC             Encoder power supply +5V          Red
                         2              GND               Encoder power ground           Yellow
                         3               /                          /                       /
                         4               /                          /                       /
                         5              SD+                 Encoder signal+               Blue
                         6              SD-                  Encoder signal-             Black
 Schematic diagram of motor terminals

                    ① ② ③                     ① ②                                           ①
                                                                        ① ②
                    ④ ⑤ ⑥                     ③ ④ ⑤                                     ③         ②
                                                                        ③ ④                 ④
                    ⑦ ⑧ ⑨                     ⑥ ⑦


              9-pin Amp plug male         7-pin aviation plug       4-pin Amp plug     4-pin aviation plug
 Motor encoder terminal pin definition (Amp plug is the same as the aviation plug)
                    Motor side                                   Name                         Wire color
                1             PE                             Shielded cable
                2             E-                    Battery power supply negative                 White
                3             E+                    Battery power supply positive                 Green
                4            SD-                             Encoder signal-                      Black
                5            GND                        Encoder power ground                    Yellow
                6            SD+                             Encoder signal+                      Blue
                7            VCC                      Encoder power supply +5V                     Red
 Motor power line pin definition
               Motor side (Amp plug)                         Name                        Wire color
                1             U                        Motor phase U                        Brown
                2             V                        Motor phase V                         Blue
                3             W                        Motor phase W                        Yellow
                4             PE                       Motor housing                        Green

4.3.4 RS485 Terminal Definition（CN4/CN5）
       Pin             color                   Signal definition
                                                                                Pin1              Pin8
        1           White/orange                    GND
        2             Orange                           /
        3           White/green                        /
        4              Blue                         485+
        5            White/blue                      485-
        6             Green                            /
        7           White/brown                        /
        8             Brown                            /

4.4 Control signal terminal wiring
4.4.1 DI input circuit
 NPN type input wiring
                              PLC                                              Servo
                                    COM
                                                   DICOM
                                       24V
                                                       DIx
                                          Y
                                       0V

                                                                                                             -9-


---

<!-- PDF page 14 -->

EtherCAT 总线伺服驱动器用户手册
 PNP type input wiring
                                   PLC                                                         Servo
                                         COM
                                                                 DICOM
                                          24V
                                                                   DIx
                                             Y
                                           0V

 Switch input wiring
                                                                                  Servo

                                                       0V
                                                 +24VO
                                                 DICOM
                                                       DIx
                                          Switch


4.4.2 DO output circuit
 DO output wiring (connect to optocoupler)
             Servo                                                                         Servo
                                                 24V                                                              DOxA
                                                                                                                                 24V
                                  DOxA                                                                            DOxB

                                  DOxB
                                                 0V                                                                               0V



                     Low level output wiring                                                           High level output wiring
 DO output wiring (connect to relay)
                                         Servo
                                                                           24V      Relay normally open contact
                                                                    DO2A                                    +

                                                                    DO2B                                 Motor brake
                                                                             Relay coil
                                                                                                              -
                                                                                          0V
                                The brake output signal controls the motor brake through the relay

4.5 Detailed description of DI/DO port function configuration
4.5.1 DI function description
1. DI port configuration parameters
          DI port                                      Function selection                                                  Logic level
                          No.         Initial value                      Function description                      No.            Initial value
           DI0           P03.02            14                Forward overtravel switch                            P03.03                 0
           DI1           P03.04            15                Reverse overtravel switch                            P03.05                 0
           DI2           P03.06            31                Origin switch                                        P03.07                 0
           DI3           P03.08             0                -                                                    P03.09                 0
           DI4           P03.10             0                -                                                    P03.11                 0

   -10-


---

<!-- PDF page 15 -->

EtherCAT 总线伺服驱动器用户手册
2. DI port function command table
                                 Function
  Code          Name                                         Description                                         Remarks
                                   name
                                Forward
                                               Effective - prohibit forward drive;
FunIN.14        P-OT            overtrave                                             When the mechanical movement exceeds the movable
                                               Invalid - allows forward drive.
                                l switch                                              range, enter the overtravel prevention function. The logical
                                Reverse                                               selection of the corresponding terminal is recommended to
                                               Effective - prohibit reverse drive;
FunIN.15        N-OT            overtrave                                             be set to: effective level.
                                               Invalid - reverse drive allowed.
                                l switch
                                                                                      The logical selection of the corresponding terminal must be
                                                                                      set to: effective level.
                                                                                      If set to 2 (effective rising edge), the internal drive will be
                                Origin         Invalid - not triggered;               forcibly changed to 1 (effective high level); If set to 3
FunIN.31     HomeSwitch
                                switch         Effective - triggered.                 (effective falling edge), the internal drive will be forcibly
                                                                                      changed to 0 (effective low-level); If set to 4 (both rising
                                                                                      and falling edges are effective), the driver will be forced to
                                                                                      change to 0 internally (low level is effective)
                                Emergen        Effective - position lock after zero   The logical selection of the corresponding terminal is
FunIN.34 Emergency Stop
                                cy stop        speed shutdown;                        recommended to be set to: effective level.
                                               Invalid - probe not triggered;         The probe logic is only related to the probe function
FunIN.38     TouchProbe1        Probe 1
                                               Effective - Probe can trigger          (60B8h) and is not related to the terminal logic selection.
                                               Invalid - probe not triggered;         The probe logic is only related to the probe function
FunIN.39     TouchProbe2        Probe 2
                                               Effective - Probe can trigger          (60B8h) and is not related to the terminal logic selection.

4.5.2 DO function description
1. DO port configuration parameters
   DI port                                           Function selection                                                Logic level
                      No.            Initial value                   Function description                      No.            Initial value
     DO0             P04.00                1            Servo ready                                           P04-01                0
     DO1             P04.02                5            Positioning completed                                 P04-03                0
     DO2             P04.04                3            Zero speed                                            P04-05                0
2. DO port function command table

   Code         Name          Function name                                                   Description
                                                    The servo status is ready and can receive S-ON valid signals: valid - the servo is ready;
 FunOUT.1     S-RDY           Servo ready
                                                    Invalid - servo not ready.
                              Motor rotation        When the speed is higher than 2006-11h: valid - the motor rotation signal is valid; Invalid -
 FunOUT.2     TGON
                              output                The motor rotation signal is invalid.
                              Positioning           When controlling the position, the position deviation pulse reaches the positioning
 FunOUT.5     COIN
                              completed             completion threshold of 6067 hours, and the time reaches 6068 hours, which is effective.
                              Torque                Confirmation signal for torque limitation: effective - motor torque limitation; Invalid -
 FunOUT.7     C-LT
                              limitation            motor torque is not limited.
                                                    Confirmation signal for speed limitation during torque control: effective - motor speed
 FunOUT.8     V-LT            Speed limit
                                                    limitation; Invalid - motor speed not limited.
                                                    Holding brake signal output: effective - closed, releasing the holding brake; Invalid -
 FunOUT.9     BK              brake output
                                                    activate the brake.
FunOUT.10 WARN                Warning output        The warning output signal is valid. (Conduction).
FunOUT.11 ALM                 Fault output          The status is valid when a fault is detected.
FunOUT.12 ALMO1               alarm code 1          Output a 3-digit alarm code.
FunOUT.13 ALMO2               alarm code 2          Output a 3-digit alarm code.
FunOUT.14 ALMO3               alarm code 3          Output a 3-digit alarm code.
                              Torque reaches        Effective - the absolute torque value reaches the set value;
FunOUT.18 ToqReach
                              output                Invalid - The absolute torque value is less than the set value reached.
                              Speed output          Effective - Speed feedback reaches the set value;
FunOUT.19 V-Arr
                              reached               Invalid - Speed feedback did not reach the set value.



                                                                                                                                                 -11-


---

<!-- PDF page 16 -->

EtherCAT 总线伺服驱动器用户手册

                        Chapter V Panel Display and Operation

5.1        Panel introduction and description
5.1.1 Description of panel keys



                                                        M                                         SET



                                                        Mode Key

                                                                   Up Key

                                                                            Down Key

                                                                                       Left Kye

                                                                                                  Set Key
            Name                                                                       General function

             M          Switch between modes, return to the previous menu

                        Increase the value of the blinking digit of the LED nixie tube

                        Decrease the value of the blinking digit of the LED nixie tube

                        Change the blinking position of the LED nixie tube, View the high-order value of data longer than 5 digits

            SET         Go to next level menu, Execute commands such as storing parameter values


5.1.2 How to operate keys on the panel

              Servo          Status
                                              M
             power on        display                       P00 group


                                                                                                  SET                   SET
                                                                                                            Parameter         Parameter
                                                           P01 group                                         P01.02            values
                                                                                                   M                     M



                                                           P31 group



5.1.3 Status display
   The servo parameter number of this series consists of two parts: parameter group and internal parameter group
number, as shown in the following figure:




                                                     Parameter
                                                       group                           Group ID

   Object dictionary index = 0x2000 + parameter group number;
   Object dictionary subindex = hexadecimal + 1 of the number within the parameter group;
   For example：the object dictionary index for P02.03 is 2002-03h, and the object dictionary index for P0B.17 is
200b-12h.
    -12-


---

<!-- PDF page 17 -->

EtherCAT 总线伺服驱动器用户手册
5.1.4 Status display




                                                  ①     ②       ③         ④


    NO.                 Display                  Name               Display occasion                      Explain

                                                 Port 1
                                               connection
                                               indication                              Long dark: No communication connection
                                                                The drive is ready
                                                                                       detected in the physical layer
    ①                                                           Servo enable signal
                                                                                       Changliang: The physical layer has
                                                 Port 0         effective
                                                                                       established a communication connection
                                               connection
                                               indication

                                                                                       The EtherCAT state machine status of the
                                                                                       slave station.
                                                                The drive is ready
                                             Communication                             1: Initialization status
    ②                                           status
                                                                Servo enable signal
                                                                                       2: Preoperational status
                                                                effective
                                                                                       4: Safe operation status
                                                                                       8: Running status
                                                                                       The current operating mode of the servo does
                                                                                       not flash.
                                                                                       0: No Mode
                                                                                       1: Contour position control
                                                                The drive is ready
                                                                                       3: Contour speed mode
    ③                                         control model     Servo enable signal
                                                                                       4: Contour torque mode
                                                                effective
                                                                                       6: Zero return mode
                                                                                       8: Periodic synchronization position mode
                                                                                       9: Periodic synchronization speed mode
                                                                                       A: Periodic synchronous torque mode
                                                                Servo initialization
                                                   Nr                                  Due to the main circuit not being powered on,
                                                                completed, but the
                                             Servo not ready                           the servo is in an inoperable state.
                                                                driver is not ready.
                                                                                       The servo drive is in a operable state, waiting
                                                   Ry
    ④                                                           The drive is ready.    for the upper computer to provide a servo
                                               Servo ready
                                                                                       enable signal.
                                                   Rn           Servo enable signal
                                                                                       The servo drive is in operation.
                                             Servo is running   effective


5.1.5 Parameter value display
    Signed numbers up to 4 digits or unsigned numbers up to 5 digits
Using a single page (5-digit digital tube) display, for signed numbers, the highest digit "-" in the data represents a
negative sign. For example, -9999 displays as follows:



Example: 65535 displays as follows:



    4 or more signed numbers or 5 or more unsigned numbers
Page by page display from low to high, with every 5 digits representing one page. Display method: current page+current
page value, as shown in the following figure. Press and hold the "" key for more than 2 seconds to switch to the current
page. For example, -268435456 is displayed as follows:
                                                                                                                                   -13-


---

<!-- PDF page 18 -->

EtherCAT 总线伺服驱动器用户手册

                                       Flashing bit indicates the number of pages



                                                    2S                                    2S
                              Page 3                                Page 2                                Page 1

    Modify P05-02 to set the default number of pulses per cycle to 10000 and set it to 1000 (modify other numerical
     parameters with more than 4 digits in the same way as this step)

                         M                                            SET

                                                                                                          press
                                                                                                         3 times

                                 SET

                                                                     press                                press
                                                                    1 times                              1 times


5.1.6 Monitoring parameter display (P0B group parameters)
    Function
                      Name                                         Setting Range                                   Unit      Attribute   Type
      code
                                       The actual running speed of the servo motor, which is rounded to the
    P0B.00 Actual motor speed                                                                                        rpm       RO        Int16
                                       nearest 1rpm
    P0B.01     Speed command           The current speed command of the drive                                        rpm       RO        Int16
               Internal torque         The percentage of the actual output torque of the servo motor to the rated
    P0B.02                                                                                                            %        RO        Int16
               command                 torque of the motor
                                       Corresponding level status of the 5 DI terminals: the upper half of the
    P0B.03     DI signal monitoring nixie tube lights up to indicate a high level; the lower half lights up to         -       RO        Int16
                                       indicate a low level
                                       Corresponding level status of the 3 DO terminals: the upper half of the
    P0B.05     DO signal monitoring nixie tube lights up to indicate a high level; the lower half lights up to         -       RO        Int16
                                       indicate a low level
               Absolute position
                                                                                                                  command
    P0B.07     counter (32-bit         Current absolute position of the motor (command unit)                                   RO        Int32
                                                                                                                     unit
               decimal display)
               Input position                                                                                     command
    P0B.13                             Display the number of input position commands                                           RO        Int32
               command count                                                                                         unit
               Encoder position        Encoder position deviation = total number of input position commands - encoder
    P0B.15                                                                                                                     RO        Int32
               deviation value         total number of encoder feedback pulses                                       unit
               Feedback pulse          Count and display the number of pulses fed back by the servo motor          encoder
    P0B.17                                                                                                                     RO        Int32
               counter                 encoder (encoder unit)                                                        unit
               Phase current rms
    P0B.24                             Servo motor phase current rms value                                            A        RO        Uint16
               value
    P0B.26     Bus voltage value       The DC bus voltage value of the main circuit                                   V        RO        Uint16
                                       Set the number of times to view historical faults
                                       0. current fault
                                       1. Last fault
    P0B.33     Fault recording                                                                                         -       RO        Uint16
                                       2. Last two faults
                                       ……
                                       9. Last 9 faults
               Fault code of selected P0B-33 selected fault code
    P0B.34                                                                                                             -       RO        Uint16
               time                    When there is no fault, the displayed value of P0B-34 is "Er.000"
               Selected fault          P0B-34 shows the total servo running time when the fault occurs
    P0B.35                                                                                                             s       RO        Int32
               timestamp               When there is no fault, the displayed value of P0B-35 is "0"
               Motor speed at          The servo motor speed when the fault displayed by P0B-34 occurs
    P0B.37                                                                                                           rpm       RO        Int16
               selected fault          When there is no fault, the displayed value of P0B-37 is "0"
               Motor U-phase           The rms value of the U-phase winding current of the servo motor when
    P0B.38     current at the selected the fault displayed by P0B-34 occurs                                           A        RO        Int16
               fault                   When there is no fault, the displayed value of P0B-38 is "0"


     -14-


---

<!-- PDF page 19 -->

EtherCAT 总线伺服驱动器用户手册
           Motor V-phase            The rms value of the V-phase winding current of the servo motor when
    P0B.39 current at the selected the fault displayed by P0B-34 occurs                                         A      RO   Int16
           fault                    When there is no fault, the displayed value of P0B-39 is "0"
                                    The DC bus voltage value of the main circuit when the fault displayed
           Bus voltage at
    P0B.40                          by P0B-34 occurs                                                            V      RO   Uint16
           selected fault
                                    When there is no fault, the displayed value of P0B-40 is "0"
                                    The corresponding high and low level status of 9 DI terminals when the
                                    fault displayed by P0B-34 occurs
           Input terminal status
    P0B.41                          Viewing method is the same as P0B-03                                         -     RO   Uint16
           at selected fault
                                    When no fault occurs, P0B-41 shows that all DI terminals are low level,
                                    and the corresponding decimal value is "0"
                                    The corresponding high and low level status of 5 DO terminals when the
                                    fault displayed by P0B-34 occurs
           Output terminal
    P0B.42                          Viewing method is the same as P0B-05                                         -     RO   Uint16
           status at selected fault
                                    When no fault occurs, P0B-42 shows that all DO terminals are low level,
                                    and the corresponding decimal value is "0"
                                    Position deviation=total number of input position instructions
           Position deviation                                                                               command
    P0B.53                          (instruction units) - total number of encoder feedback pulses                      RO   Int32
           counter                                                                                             unit
                                    (instruction units)
    P0B.55 Actual motor speed The actual running speed of the servo motor, accurate to 0.1rpm                  rpm     RO   Int32
           Control power bus
    P0B.57                          Control circuit DC bus voltage value                                         -     RO   Uint16
           voltage
           Mechanical absolute                                                                               encoder
    P0B.58                          Mechanical corresponding position feedback low 32 bit value                        RO   Int32
           position(Low 32-bit)                                                                                unit
           Mechanical absolute                                                                               encoder
    P0B.60                          Mechanical corresponding position feedback high 32 bit value                       RO   Int32
           position(High 32-bit)                                                                               unit
           Input position           Display position command counter before electronic gear ratio           command
    P0B.64                                                                                                             RO   Int32
           instruction count        multiplication                                                             unit
           Absolute encoder
    P0B.70                          Display the number of rotations of the absolute value encoder                r     RO   Uint16
           rotation number
           Absolute encoder         Display the single loop position feedback value of the absolute value    encoder
    P0B.71                                                                                                             RO   Int32
           position within 1 turn encoder                                                                      unit
           Absolute encoder         Display the position feedback value of the absolute value encoder, with encoder
    P0B.77                                                                                                             RO   Int32
           position (Low 32-bit) low 32-bit data                                                               unit
           Absolute encoder
                                    Display the position feedback value of the absolute value encoder, with encoder
    P0B.79 position (High                                                                                              RO   Int32
                                    high 32-bit data                                                           unit
           32-bit)
           Rotating load single
                                                                                                             encoder
    P0B.81 circle position          Position feedback value of rotating load, low 32-bit data                          RO   Uint32
                                                                                                               unit
           (Low 32-bit)
           Rotating load single
                                                                                                             encoder
    P0B.83 circle position          Position feedback value of rotating load, high 32-bit data                         RO   Uint32
                                                                                                               unit
           (High 32-bit)
           Rotating load single                                                                             command
    P0B.85                          Position feedback value of rotating load, high 32-bit data                         RO   Uint32
           circle position                                                                                     unit


5.2 Common operations
                                                               Warning
● Please check whether the wiring of the drive is correct before powering on.
● Make sure that the motor is not loaded to prevent collision or other hazards.
5.2.1 Initialization parameters
Set P02-31 to 1 to initialize the drive parameters, and the drive needs to be restarted after the setting is completed.
5.2.2 Manual reset alarm
          Set P0D-01 to 1 to clear the resettable alarms;
          For multi-turn absolute encoder power failure alarm (Er.731), first set P0D-20 to 2, and then set P0D-01 to 1 to
           clear the alarm.
5.2.3 JOG mode operation
When using the jog function, you need to cancel the servo enable first, or you can't enter the JOG state!
                                                                                                                                    -15-


---

<!-- PDF page 20 -->

EtherCAT 总线伺服驱动器用户手册

               Power                              M
                on
                                                                                                       SET




                                                                                                       SET


                                                   JOG speed can be modified by Up
                                                           and Down keys                                              M
                                                                                              2        SET
                                                                                           seconds




                                                                                        Press       The motor is
                                                                                                  turning forward
                                                                                                    The motor is
                                                                                        Press     turning reversal



5.3 Description of Gain Parameter Setting
                                   Setting
  No.       Parameter name                                                           Functions
                                    range
                                             The larger this parameter is, the faster the response of the speed loop will be, but the
                                   0.1~      setting too large may cause vibration;
 P08-00   Speed loop gain
                                   2000.0    In position mode, to increase the gain of position loop, it is necessary to increase the gain
                                             of speed loop at the same time.
                                             The smaller the value set, the stronger the integration effect, the faster the response, the
          Speed loop integral      0.15~     large inertia load may cause jitter;
 P08-01
          time constant            512.00    The larger the setting value is, the slower the response will be. Increase this parameter
                                             appropriately with large inertia load.
                                             This parameter determines the responsiveness of the position loop, and a larger position
                                   0.0~
 P08-02   Position loop gain                 loop gain can shorten the positioning time. However, setting too large may cause
                                   2000.0
                                             vibration.
                                             Set the mechanical load inertia ratio relative to the motor's own moment of inertia.
                                   0.00~
 P08-15   Load inertia ratio                 When the motor drives a large inertia load such as belt/rack and pinion/swing arm, this
                                   120.00
                                             parameter can be increased if there is rocking back and forth.
          Speed feedforward        0.00~
 P08-18                                      Sets the filtering time constant for velocity feedforward.
          filter time constant     64.00
                                             Increasing this parameter can improve the position command response and reduce the
                                             position deviation at fixed speed.
          Speed feedforward        0.0~
 P08-19                                      When adjusting, first set P08-18 as a fixed value; Then the set value of P08-19 is
          gain                     100.0
                                             gradually increased from 0 until a certain set value, the speed feedforward effect is
                                             achieved.
          Torque feedforward       0.00~
 P08-20                                      Set the filtering time constant for torque feedforward.
          filter time constant     64.00
                                             Increasing this parameter improves responsiveness to changing speed instructions.
          Torque feedforward       0.0~
 P08-21                                      Increasing this parameter can improve the position command response and reduce the
          gain                     200.0
                                             position deviation at fixed speed.
          Speed feedback
                                   100~      The smaller the setting, the smaller the speed feedback fluctuation, but the larger the
 P08-23   low-pass filter cutoff
                                   4000      feedback delay.
          frequency
                                             When the coefficient is set to 100.0, the speed loop adopts PI control (the default control
                                             mode of the speed loop), and the dynamic response is fast.
          Pseudo-differential
                                   0.0~      When it is set to 0.0, the speed loop integration has an obvious effect and can filter out
 P08-24   feedforward control
                                   100.0     low-frequency interference, but the dynamic response is slow.
          coefficient
                                             By adjusting P08-24, the speed loop can not only have fast response, but also not increase
                                             the speed feedback overshoot, but also improve the immunity of low frequency band.

   -16-


---

<!-- PDF page 21 -->

EtherCAT 总线伺服驱动器用户手册

                                   Chapter VI Communication configuration

6.1 EtherCAT networking diagram
     EtherCAT is a high-performance, low-cost, easy-to-use, and topologically flexible industrial Ethernet technology that
can be used for ultra high speed I/O networks at the industrial field level. It uses standard Ethernet physical layers and
transmission media twisted pair or fiber optic (100Base TX or 100Base FX). The EtherCAT network diagram is as follows:


                                                                                      EtherCAT
                                                                                     controller




                             A C 220V                             A C 220V                      A C 220V                      A C 220V                      A C 220V




                IN     OUT                             IN   OUT                      IN   OUT                      IN   OUT                      IN   OUT
         C N1




                                                C N1




                                                                              C N1




                                                                                                            C N1




                                                                                                                                          C N1
         C N2




                                                C N2




                                                                              C N2




                                                                                                            C N2




                                                                                                                                          C N2
        CN3




                                               CN3




                                                                             CN3




                                                                                                           CN3




                                                                                                                                         CN3
                        L                                   L                             L                             L                             L
                        N                                   N                             N                             N                             N
                       B1                                   B1                            B1                            B1                            B1
                       B3                                   B3                            B3                            B3                            B3
                       B2                                   B2                            B2                            B2                            B2
                       U                                    U                             U                             U                             U
                       V                                    V                             V                             V                             V
                       W                                    W                             W                             W                             W
                       PE                                   PE                            PE                            PE                            PE
                       PE                                   PE                            PE                            PE                            PE




6.1.1 EtherCAT communication technology specifications
                                        Item                                                                            specifications
                     Communication protocol                                          EtherCAT protocol
                     Support services                                                CoE （PDO、SDO）
                     Synchronization method                                          DC Distributed Clock
                     physical layer                                                  100BASE-TX
                     Baud rate                                                       100 Mbit/s (100Base-TX)
                     Duplex mode                                                     full duplex
 EtherCAT            topological structure                                           linear
   Slave                                                                             Shielded Category 5 or Electrical Performance Specification Category 6 or
                     Transmission medium
  Station                                                                            above Ethernet cables
Performance                                                                          The distance between two nodes is less than 100m (with good environment
                     transmission distance
                                                                                     and excellent cables)
                                                                                     Protocol supports up to 65535 units, but actual usage does not exceed 100
                     Number of slave stations
                                                                                     units
                     EtherCAT frame length                                           44 bytes to 1498 bytes
                     process data                                                    A maximum of 1486 bytes per Ethernet frame
                     Synchronous jitter between two slave stations                   < 1us



                                                                                                                                                                       -17-


---

<!-- PDF page 22 -->

EtherCAT 总线伺服驱动器用户手册

6.2 Driver related parameter configuration

6.2.1 system parameter setting
     In order to accurately connect this series of servo drives to the EtherCAT fieldbus network, it is necessary to set the
relevant parameters of the servo drives. As shown in the table below:
Function                                                                                                Default         Effective     Setting   Related
           Index Subindex         Name                           Set Range                   Unit
  code                                                                                                  settings         method       method    modes

                            Control mode                                                                                Effective No enable
 P02.00    2002h    01                           9：EtherCAT                                       -          9                                     -
                            selection                                                                                 immediately settings
                                                 0: Do not save
                                                 1: 2000h series object dictionary
                            Is the               communication written and stored in
                            communication        EEPROM
                            writing function     2: 6000h series object dictionary                                      Effective Running
 P0C.13    200Ch    0E                                                                            -          3                                   PST
                            code value           communication written and stored in                                  immediately settings
                            updated to           EEPROM
                            EEPROM               3: 2000h series and 6000h series object
                                                 dictionaries are stored in EEPROM after
                                                 communication and writing

     Attention: The parameters that need to be saved in EEPROM must be set to the corresponding values of 200C-0Dh
before setting. Otherwise, after re powering on, the parameters will return to their default values.

6.2.2 Rotation direction selection
    By setting the rotation direction selection (2002-03h) or P02-02, the rotation direction of the motor can be changed
without changing the polarity of the input command. The relevant parameters are shown in the table below

Function                                                                                                 Default         Effective   Setting    Related
           Index Subindex       Name                          Set Range                      Unit
  code                                                                                                   settings         method     method     modes
                                               0: Using the CCW direction as the forward
                             Rotation                                                                                                   No
                                               turning direction (A leads B)                                              Power
 P02.02    2002h    03       direction                                                           -               0                    enable     PST
                                               1: Take CW direction as the forward                                       on again
                             selection                                                                                               settings
                                               rotation direction (A lags behind B)

When the rotation direction selection (2002-03h) is changed, the shape of the servo driver's output pulse and the positive
or negative monitoring parameters will not change.

6.2.3 Holding brake setting
     Band brake is a mechanism that prevents the servo motor shaft from moving when the servo drive is in a non
operating state, keeping the motor position locked and preventing the moving parts of the machinery from moving due to
self weight or external forces. Under relevant parameters:

Function                                                                                              Default         Effective      Setting    Related
         Index Subindex                          Name                        Set Range     Unit
  code                                                                                                settings         method        method     modes
                             Holding brake output ON to command                                                       Effective      Running
 P02.09    2002h    0A                                                        0～500        ms           250                                       PS
                             reception delay                                                                         immediately     settings
                             Static state, holding brake output OFF to                                                Effective      Running
 P02.10    2002h    0B                                                       1～1000        ms           150                                       PS
                             motor power-off delay                                                                   immediately     settings
                             Rotating state, holding brake output                                                     Effective      Running
 P02.11    2002h    0C                                                       0～3000        rpm          30                                        PS
                             OFF speed threshold                                                                     immediately     settings
                             Rotation state, servo enable OFF to                                                      Effective      Running
 P02.12    2002h    0D                                                       1～1000        ms           500                                       PS
                             brake output OFF delay                                                                  immediately     settings

The output signal of the brake is controlled by a relay, and the wiring diagram of the brake is shown in Chapter 4.4.2.
    -18-


---

<!-- PDF page 23 -->

EtherCAT 总线伺服驱动器用户手册

6.3 Communication cycle of each mode

               Profile position     Zero return         Periodic         Periodic          Profile Speed     Contour torque      Periodic
  Cycle             mode              mode            synchronous      Synchronous            Mode               mode          Synchronous
  time               (PP)             (HM)           position mode     Speed Mode               (PV)             (PT)          Torque Mode
                                                          (CSP)           (CSV)                                                   (CST)
  125us               ×                  ×                 ×                   ×                 ×                 √                   √
  250us               ×                  ×                 ×                   ×                 ×                 √                   √
  500us               ×                  ×                 ×                   √                 √                 √                   √
   1ms                √                  √                 √                   √                 √                 √                   √
The synchronization cycles supported by modes of 1ms and below are shown in the table above. When used outside of
the specifications, it may cause operational errors;
A synchronization period that is an integer multiple of the position loop control cycle (position loop control cycle is
250us) and is greater than 1ms can also be supported.

6.4 Process Data PDO
6.4.1 Variable PDO mapping
This series of drivers provides 1 variable RPDO and 1 variable TPDO for users to use. As shown in the table below:
    Variable                                                         Longest
                     Index        Maximum number of mappings                                         Default mapping object
     PDO                                                              Byte
                                                                                   6040h (control word)
    RPDO1            1600h                     10                      40          607Ah (target position)
                                                                                   60B8h (probe function)
                                                                                   603Fh (error code)
                                                                                   6041h (status word)
                                                                                   6064h (position feedback)
    TPDO1           1A00h                      10                      40          60BCh (probe 2 rising edge position feedback)
                                                                                   60B9h (probe status)
                                                                                   60BAh (feedback on the rising edge position of probe 1)
                                                                                   60FDh (DI status)

6.4.2 Fixed PDO mapping
This series of drives provides 5 fixed RPDOs and 4 fixed TPDOs for use. As shown in the table below:
   PDO         Supported servo                                         PDO            Supported servo
   group                             PP/CSP                            group                                 PP/PV/PT/CSP/CSV/CST
                   modes                                                                  modes
                                     Mapping objects (3 x 8 bytes)                                           Mapping object (7 19 bytes)
                                                                                                             6040h (control word)
                                                                                                             607Ah (target position)
                   1701h             6040h (control word)                                                    60FFh (target speed)
                                                                                      1702h(RPDO259)
                 (RPDO258)           607Ah (target position)                                                 6071h (target torque)
                                     60B8h (probe function)                                                  6060h (mode selection)
                                                                                                             60B8h (probe function)
                                                                                                             607Fh (maximum speed)
                                     Mapping objects (8 24 bytes)                                            Mapping objects (9 of 25 bytes)
     1                                                                   2
                                                                                                             603Fh (error code)
   group                                                               group
                                     603Fh (error code)                                                      6041h (status word)
                                     6041h (status word)                                                     6064h (position feedback)
                                     6064h (position feedback)                                               6077h (torque feedback)
                   1B01h             6077h (torque feedback)                                                 6061h (mode display)
                                                                                     1B02h(TPDO259)
                 (TPDO258)           60F4 (positional deviation)                                             60B9 (probe status)
                                     60B9 (probe status)                                                     60BA (Probe 1 rising edge
                                     60BA (Probe 1 rising edge                                               position feedback)
                                     position feedback)                                                      60BC (probe 2 rising edge
                                     60FD (DI status)                                                        position feedback)
                                                                                                             60FD (DI status)
                                                                                                                                             -19-


---

<!-- PDF page 24 -->

EtherCAT 总线伺服驱动器用户手册


PDO     Supported servo                                   PDO       Supported servo
                                      PP/CSP                                           PP/PV/PT/CSP/CSV/CST
group       modes                                         group         modes
                          Mapping object (7 17 bytes)                                  Mapping object (7 17 bytes)
                                                                                       6040h (control word)
                          6040h (control word)                                         607Ah (target position)
                          607Ah (target position)                                      60FFh (target speed)
        1703h(RPDO260)    60FFh (target speed)                      1704h(RPDO261)     6071h (target torque)
                          6060h (mode selection)                                       6060h (mode selection)
                          60B8h (probe function)                                       607Fh (maximum speed)
                          60E0h (forward torque limit)                                 60B8h (probe function)
                          60E1h (Negative torque limit)                                60E0h (forward torque limit)
                                                                                       60E1h (Negative torque limit)

  3                       Mapping objects (10 29 bytes)                                Mapping objects (9 of 25 bytes)
                          603Fh (error code)              4 group
group                                                                                  603Fh (error code)
                          6041h (status word)
                                                                                       6041h (status word)
                          6064h (position feedback)
                                                                                       6064h (position feedback)
                          6077h (torque feedback)
                                                                                       6077h (torque feedback)
                          60F4h (position deviation)
        1B03h(TPDO260)                                              1B02h(TPDO259)     6061h (mode display)
                          6061h (mode display)
                                                                                       60B9 (probe status)
                          60B9h (probe status)
                                                                                       60BA (Probe 1 rising edge
                          60BAh (feedback on the rising
                                                                                       position feedback)
                          edge position of probe 1)
                                                                                       60BC (probe 2 rising edge
                          60BCh (probe 2 rising edge
                                                                                       position feedback)
                          position feedback)
                                                                                       60FD (DI status)
                          60FD (DI status)


PDO     Supported servo
                          PP/CSP
group       modes
                          Mapping objects (8 19 bytes)                        Mapping objects (10 29 bytes)
                                                                              603Fh (error code)
                                                                              6041h (status word)
                          6040h (control word)                                6064h (position feedback)
                          607Ah (target position)                             6077h (torque feedback)
  5                       60FFh (target speed)                                6061h (mode display)
        1705h(RPDO262)    6060h (mode selection)          1B04h(TPDO261)      60F4h (position deviation)
group
                          60B8h (probe function)                              60B9h (probe status)
                          60E0h (forward torque limit)                        60BAh (feedback on the rising edge
                          60E1h (Negative torque limit)                       position of probe 1)
                          60B2h (torque bias)                                 60BCh (probe 2 rising edge position
                                                                              feedback)
                                                                              606Ch (speed feedback)




 -20-


---

<!-- PDF page 25 -->

EtherCAT 总线伺服驱动器用户手册

                             Chapter VII Control mode description

7.1 Introduction to Control
     To use this series of drives, the servo drive must be guided according to the process specified in the standard 402
protocol in order to operate in the specified state. The description of each state is shown in the table below:
                                      Driver initialization and internal self check have been completed.
           Initialization
                                      The parameters of the driver cannot be set or the driver function cannot be executed.

                                      The servo drive has no faults or errors have been resolved.
          Servo fault free
                                      The drive parameters can be set.

                                      The servo drive is ready.
            Servo ready
                                      The drive parameters can be set.

     Waiting to turn on servo         The servo driver is waiting to turn on the servo enable.
              enable                  The drive parameters can be set.
                                      When the driver is running normally, a certain servo operating mode has been enabled, the motor
          Servo operation             is powered on, and the command is not 0, the motor rotates.
                                      The drive parameter attribute can be set to "Run Change", while others cannot.
                                      The quick stop function has been activated, and the driver is executing the quick stop function.
             Fast stop
                                      The drive parameter attribute can be set to "Run Change", while others cannot.

                                      The drive has malfunctioned and is currently undergoing a fault shutdown process
          Fault shutdown
                                      The drive parameter attribute can be set to "Run Change", while others cannot.
                                      The fault shutdown is completed, and all drive functions are disabled. At the same time, it is
               Fault
                                      allowed to change the drive parameters to troubleshoot

     The switching between control commands and status words is shown in the table below:
                                                                                                                      Status word 6041h
                    CiA402 State switching                                 Control word 6040h value
                                                                                                                          bit0～bit9
                                                                     Natural transition without the need for
     0    Power on → initialization                                                                                        0x0000
                                                                     control commands
                                                                     Natural transition without the need for
                                                                     control commands
     1    Initialize → Servo has no faults                                                                                 0x0250
                                                                     If an error occurs during initialization,
                                                                     directly enter 13
     2    Servo fault free → Servo ready                                                   6                               0x0231
     3    Servo ready → waiting to turn on servo enable                                    7                               0x0233
     4    Waiting to turn on servo enable → Servo running                                  F                               0x0237
     5    Servo operation → wait to turn on servo enable                                   7                               0x0233
     6    Waiting to turn on servo enable → servo ready                                    6                               0x0231
     7    Servo ready → Servo free from malfunction                                        0                               0x0250
     8    Servo operation → Servo ready                                                    6                               0x0231
     9    Servo operation → Servo has no faults                                            0                               0x0250
    10    Waiting to turn on servo enable → Servo has no faults                            0                               0x0250
    11    Servo operation → Quick stop                                                     2                               0x0217
                                                                     The fast shutdown mode 605A is selected as
                                                                     0-3, and after the shutdown is completed, it
    12    Quick stop → Servo no fault                                                                                      0x0250
                                                                     will naturally transition without the need for
                                                                     control commands
                                                                     In any other state except for "fault", once
                                                                     the servo drive fails, it automatically
    13    Fault shutdown                                                                                                   0x021F
                                                                     switches to the fault shutdown state
                                                                     without the need for control commands
                                                                                                                                          -21-


---

<!-- PDF page 26 -->

EtherCAT 总线伺服驱动器用户手册
                                                                         After the fault shutdown is completed, it
   14      Fault shutdown → Fault                                        transitions naturally without the need for        0x0218
                                                                         control commands
                                                                         0x80
                                                                         Bit7 rising edge effective;
   15      Fault → No servo fault                                                                                          0x0250
                                                                         Bit7 remains at 1, all other control
                                                                         instructions are invalid.
                                                                         The fast shutdown mode 605A is
   16      Quick stop → servo operation                                  selected as 5-7. After the shutdown is            0x0237
                                                                         completed, send 0x0F

7.1.1 Control word 6040h
           Bit              function                                                    describe
            0              switch on                                               1: Valid, 0: Invalid
            1            enable voltage                                            1: Valid, 0: Invalid
            2              quick stop                                              1: Invalid, 0: Valid
            3           enable operation                                           1: Valid, 0: Invalid
          4～6        operation mode speciﬁc                            Related to various servo operation modes
                                                 For resettable faults and warnings, perform the fault reset function;
            7              fault reset           Bit7 rising edge effective;
                                                 Bit7 remains at 1, all other control instructions are invalid.
            8                 halt                  Please refer to the object dictionary 605Dh for the pause methods in each mode
            9        operation mode speciﬁc                            Related to various servo operation modes
           10                 N/A                                                         N/A
          11～15               N/A                                                         N/A

7.1.2 Status word 6041h
           Bit 位                  function                                                   describe
                0            ready to switch on                                        1: Valid, 0: Invalid
                1                switch on                                             1: Valid, 0: Invalid
                2            operation enabled                                         1: Valid, 0: Invalid
                3                     fault                                            1: Valid, 0: Invalid
                4             voltage enabled                                          1: Valid, 0: Invalid
                5                quick stop                                            1: Invalid, 0: Valid
                6           switch on disabled                                         1: Valid, 0: Invalid
                7                 warning                                              1: Valid, 0: Invalid
                8                     N/A                                                      N/A
                9                    remote                                            1: Valid, 0: Invalid
                10              target reach                                           1: Valid, 0: Invalid
                11          internal limit active                                      1: Valid, 0: Invalid
           12～13          operation mode speciﬁc                           Related to various servo operation modes
                14                    N/A                                                      N/A
                15              Home Find                                              1: Valid, 0: Invalid




   -22-


---

<!-- PDF page 27 -->

EtherCAT 总线伺服驱动器用户手册

7.2 Working mode

7.2.1 Introduction to servo mode
    The control modes supported by this series of servo drives include:
           Index       Subindex          Name                Set value                           describe
                                                                1             Profile position mode (pp)
                                                                3             Profile Speed Mode (PV)
                                                                4             Contour torque mode (pt)
           6060h          00          Working mode              6             Zero return mode (hm)
                                                                8             Periodic synchronous position mode (CSP)
                                                                9             Periodic Synchronous Speed Mode (CSV)
                                                                10            Periodic Synchronous Torque Mode (CST)

7.2.2 Servo mode switching
     1. When the servo drive is in any state and switches from contour position mode or periodic synchronous position
mode to other modes, unexecuted position commands will be discarded.
     2. When the servo drive is in any state, after switching from contour speed mode, contour torque mode, periodic
synchronous speed mode, and periodic synchronous torque mode to other modes, it first executes a ramp stop. After the
stop is completed, it can switch to other modes.
     3. When the servo is in zero return mode and running, it cannot switch to other modes; When the return to zero is
completed or interrupted (due to malfunction or invalid enable), other modes can be switched on.
     4. When switching from other modes to periodic synchronization mode during servo operation, please send
instructions at least 1ms apart, otherwise command loss or error may occur.


7.3 Periodic synchronous position mode (CSP mode)
     Set 6060h to 8 and put the drive in CSP mode. In the periodic synchronization position mode, the upper controller
completes the position command planning, and then sends the planned target position 607Ah to the servo driver in a
periodic synchronization manner. The position, speed, and torque control are completed internally by the servo driver.
This mode is suitable for multi axis synchronous position control. The commonly used object dictionaries for using CSP
mode are as follows:
            Index    Subindex                Name                    type          data type                  unit
            603Fh        00                Error code                    RO         UINT16                      -
            6041h        00                Status word                   RO         UINT16                      -
            6061h        00               Mode display                   RO           INT8                      -
            6062h        00             Position command                 RO          INT32               Command Unit
            6064h        00             Position feedback                RO          INT32               Command Unit
            606Ch        00               Actual speed                   RO          INT32            Command Unit /S
            60F4h        00             Position deviation               RO          INT32               Command Unit
            60FCh        00             Position command                 RO          INT32                 Encoder unit
            60FDh        00              Input IO status                 RO         UINT32                      -
            6040h        00               Control word                   RW         UINT16                      -
            6060h        00              Control model                   RW           INT8                      -
                                                                                                                          -23-


---

<!-- PDF page 28 -->

EtherCAT 总线伺服驱动器用户手册
           607Ah        00                   Target position                    RW             INT32               Command Unit
            607Fh       00                  Maximum speed                       RW             UDINT               Command Unit
                                     Excessive position deviation
            6065h       00                                                      RW         UINT32                  Command Unit
                                           alarm threshold
            6067h       00            Position reaches threshold                RW         UINT32                    Encoder unit
            6068h       00             Position reaches window                  RW         UINT16                              ms
                        01                  Motor resolution                    RW         UINT32                              -
            6091h
                        02                   Axis resolution                    RW         UINT32                              -
            60B0h       00                    Position offset                   RW             INT32               Command Unit
            60B1h       00                     Speed offset                     RW             INT32              Command Unit /S
            60B2h       00                    Torque offset                     RW             INT32                      0.1%
            6072h       00                  Maximum torque                      RW         UINT16                         0.1%
            60E0h       00                 Forward torque limit                 RW         UINT16                         0.1%
            60E1h       00                 Reverse torque limit                 RW         UINT16                         0.1%


                                                  Servo ON


                                             Position offset 60B0h
                                             Target position 607Ah


                                                  Gear ratio
                                                  6091-01h
                      Speed offset 60B1h          6091-02h

                                                    +        -   Position feedback
                                                                       6063h
                                                                                                             Torque feedback
                                                                                                                 6077h
                                                                                     encoder     Motor
                      Speed feedforward
                          2008-14h
                        Speed filtering                                                   Torque filtering
                                                 Position gain        speed
                          2008-13h                                                          2007-06h
                                                  2008-03h           calculate
                                                                                         Torque limitation

                                                  + +
                                               Maximum speed
                                                                     +
                                                                            -            Speed regulator             +
                                                    limit
                                                   607Fh                               2008-01h、2008-02h
                                                                                                                      +
                                                 Speed offset              Torque feedforward gain 2008-16h
                                                   60B1h                  Torque feedforward filtering 2008-15h


                                                     CSP Control mode diagram

7.4 Periodic synchronous speed mode (CSV mode)
     Set 6060h to 9 and put the drive in CSV mode. In the periodic synchronization speed mode, the upper controller
sends the calculated target speed of 60FF to the servo driver in periodic synchronization, and the speed and torque
adjustment is performed internally by the servo. This mode is suitable for multi axis synchronous speed control. The
commonly used object dictionaries for using CSV mode are as follows:
            Index     Subindex                    Name                          type       data type                       unit

            603Fh       00                      Error code                      RO          UINT16                             -
            6041h       00                     Status word                      RO          UINT16                             -
            6061h       00                    Mode display                      RO             INT8                            -


    -24-


---

<!-- PDF page 29 -->

EtherCAT 总线伺服驱动器用户手册
            6064h         00                    Position feedback              RO            INT32              Command Unit
            606Ch         00                      Actual speed                 RO            INT32             Command Unit /S
            6077h         00                      Actual torque                RO            INT16                   0.1%
            6040h         00                      Control word                 RW           UINT16                        -
            6060h         00                      control model                RW            INT8                         -
            60FFh         00                      Target speed                 RW            INT32             Command Unit /S
            607Fh         00                    Maximum speed                  RW       UDINT32                Command Unit /S
            60B1h         00                      Speed offset                 RW            INT32             Command Unit /S
            60B2h         00                      Torque offset                RW            INT32                   0.1%
            6072h         00                    Maximum torque                 RW           UINT16                   0.1%
            60E0h         00               Forward torque limit                RW           UINT16                   0.1%
            60E1h         00                   Reverse torque limit            RW           UINT16                   0.1%

                                                          Servo ON


                                                     Speed offset 60B1h
                                                     Target speed60FFh


                                                       Maximum speed
                               Torque offset
                                                            limit
                                  60B2h
                                                           607Fh

                                                           +        Speed feedback
                                                                    -   606Ch
                                                                                                        Torque feedback
                                                                                                            6077h
                                                                                  Encoder      Motor

                         Torque feedforward gain
                                 2008-16h            Speed regulator
                        Torque feedforward filtering                                  Torque filtering
                                                       2008-01h                         2007-06h
                                 2008-15h              2008-02h                      Torque limitation

                                                         + +
                                                        CSV Control mode diagram

7.5 Periodic synchronous torque mode (CST mode)
     Set 6060h to 10, and the driver is in CST mode. In periodic synchronous torque mode, the upper controller will
periodically synchronize the calculated target torque 6071h to the servo driver, and torque adjustment is performed
internally by the servo. When the speed reaches the limit amplitude, it will enter the speed regulation stage. This mode is
suitable for multi axis synchronous torque control, and the commonly used object dictionary for CST mode is as follows:
             Index     Subindex                       Name                    type          data type                 unit
            603Fh         00                       Error code                  RO           UINT16                        -
            6041h         00                       Status word                 RO           UINT16                        -
            6061h         00                      Mode display                 RO            INT8                         -
            606Ch         00                      Actual speed                 RO            INT32             Command Unit /S
            6074h         00                    Torque command                 RO            INT16                   0.1%
            6077h         00                      Actual torque                RO            INT16                   0.1%
            6040h         00                      Control word                 RW           UINT16                        -
            6060h         00                      control model                RW            INT8                         -
            6071h         00                      Target torque                RW            INT16                   0.1%
            607Fh         00                    Maximum speed                  RW       UDINT32                Command Unit /S

                                                                                                                                 -25-


---

<!-- PDF page 30 -->

EtherCAT 总线伺服驱动器用户手册
            60B2h         00                 Torque offset                 RW         INT32             0.1%
             6072h        00               Maximum torque                  RW        UINT16             0.1%
            60E0h         00             Forward torque limit              RW        UINT16             0.1%
            60E1h         00              Reverse torque limit             RW        UINT16             0.1%

                                            Servo ON


                                       Torque offset60B2h
                                       Target torque6071h


                                      Maximum speed limit
                                           607Fh
                                                                                  Speed feedback
                                                                                      606Ch
                                             +       -               Speed regulator
                                                                        2008-01h
                                                                        2008-02h

                                     Torque filtering 2007-06h
                                      Maximum torque 6072h               Motor    Encoder


                                                       Torque feedback
                                                           6077h

                                                  CST Control mode diagram

7.6 Profile position mode (PP mode)
     Set 6060h to 1 and put the driver in PP mode, which is mainly used for point-to-point positioning applications. In
this mode, the upper computer provides the target position (absolute or relative), the speed of the position curve,
acceleration/deceleration, and deceleration. The trajectory generator inside the servo will generate target position curve
instructions based on the settings, and the driver will complete position control, speed control, and torque control
internally. The commonly used object dictionaries for using the PP mode are as follows:
             Index     Subindex                  Name                      type      data type           unit

            603Fh         00                   Error code                  RO        UINT16               -
            6041h         00                  Status word                  RO        UINT16               -
            6061h         00                 Mode display                  RO          INT8               -
            6062h         00              Position command                 RO         INT32         Command Unit
            6063h         00               Position feedback               RO         INT32          Encoder unit
            6064h         00               Position feedback               RO         INT32         Command Unit
            606Ch         00                  Actual speed                 RO         INT32        Command Unit /S
            60F4h         00               Position deviation              RO         INT32         Command Unit
            60FCh         00              Position command                 RO         INT32          Encoder unit
            60FDh         00                Input IO status                RO        UINT32               -
            6077h         00                 Actual torque                 RO         INT16             0.1%
            6040h         00                 Control word                  RW        UINT16               -
            6060h         00                 control model                 RW          INT8               -
            607Ah         00                Target position                RW         INT32         Command Unit
            6081h         00                  Target speed                 RW       UDINT32        Command Unit /S
            6083h         00                  Acceleration                 RW       UDINT32        Command Unit /S2
            6084h         00                  Deceleration                 RW       UDINT32        Command Unit /S2
                                     Excessive position deviation
            6065h         00                                               RW        UINT32         Command Unit
                                           alarm threshold
            6067h         00          Position reaches threshold           RW        UINT32          Encoder unit
    -26-


---

<!-- PDF page 31 -->

EtherCAT 总线伺服驱动器用户手册
            6068h         00                Position reaches window                     RW              UINT16                          ms
            6072h         00                    Maximum torque                          RW              UINT16                         0.1%
                          01                    Motor resolution                        RW              UINT32                           -
            6091h
                          02                      Axis resolution                       RW              UINT32                           -
            60E0h         00                  Forward torque limit                      RW              UINT16                         0.1%
            60E1h         00                  Reverse torque limit                      RW              UINT16                         0.1%


                                      Servo ON



                                Target position 607Ah
                                Contour speed 6081h
                                 Acceleration 6084h
                                 Deceleration 6083h


                                     Gear ratio
                                     6091-01h
                                     6091-02h                                                 Torque feedback
                                                                                                  6077h

                                             Position feedback
                                        +          6063h                                                         Torque filtering
                                                                                                                    2007-06h

              Speed feedforward
                                              -                  Encode         Motor
                                                                                                                Torque limitation
                                                                                                                                             + +
                  2008-14h                                                   Speed feedback
                Speed filtering     Position gain                 speed          606Ch                      Speed regulator
                  2008-13h           2008-03h                    calculate
                                                                                      -+                  2008-01h、2008-02h

                                        +
                                    +                      Maximum speed
                                                                limit
                                                                                                    Torque feedforward gain 2008-16h
                                                                                                   Torque feedforward filtering 2008-15h
                                                               607Fh

                                                          PP Control mode diagram

7.7 Profile speed mode (PV mode)
     Set 6060h to 3 and put the drive in PV mode. In this mode, the upper controller sends the target speed, acceleration,
and deceleration to the servo driver, and the speed and torque adjustment is performed internally by the servo. The
commonly used object dictionaries for using PV mode are as follows:
             Index     Subindex                        Name                             type            data type                       unit
            603Fh          00                        Error code                         RO              UINT16                           -
            6041h          00                       Status word                         RO              UINT16                           -
            6061h          00                       Mode display                        RO                INT8                           -
            6063h          00                   Position feedback                       RO               INT32                      Encoder unit
            6064h          00                   Position feedback                       RO               INT32                  Command Unit
            606Ch          00                       Actual speed                        RO               INT32                Command Unit /S
            6077h          00                       Actual torque                       RO               INT16                         0.1%
            6040h          00                       Control word                        RW              UINT16                           -
            6060h          00                      control model                        RW                INT8                           -
            60FFh          00                       Target speed                        RW               INT32                Command Unit /S
            6083h          00                       Acceleration                        RW             UDINT32                Command Unit /S2
            6084h          00                       Deceleration                        RW             UDINT32                Command Unit /S2
            607Fh          00                     Maximum speed                         RW             UDINT32                Command Unit /S
            606Dh          00                Speed reaches threshold                    RW               INT32                Command Unit /S
            606Eh          00               Speed reaches the window                    RW               INT32                          ms

                                                                                                                                                   -27-


---

<!-- PDF page 32 -->

EtherCAT 总线伺服驱动器用户手册
            60B1h         00                    Speed offset                RW            INT32           Command Unit /S
            60B2h         00                   Torque offset                RW            INT32                  0.1%
            60E0h         00               Forward torque limit             RW            UINT16                 0.1%
            60E1h         00               Reverse torque limit             RW            UINT16                 0.1%

                                                          Servo ON


                                                      Target speed 60FFh
                                                      Acceleration 6083h
                                                      Deceleration 6084h


                                                       Maximum speed
                                                            limit
                                                           607Fh

                                                                + Speed606Ch
                                                                         feedback                   Torque feedback
                                                                                                        6077h
                                                                                Encoder     Motor
                                                                 -
                        Torque feedforward gain
                                2008-16h               Speed regulator
                       Torque feedforward filtering       2008-01h
                                2008-15h                  2008-02h


                                                         + +                     Torque filtering
                                                                                    2007-06h
                                                                                Torque limitation

                                                       PV Control mode diagram

7.8 Contour torque mode (PT mode)
     Set 6060h to 4 and put the driver in PT mode. In this mode, the upper controller sends the target torque of 6071h and
the torque ramp constant of 6087h to the servo driver, and torque adjustment is performed internally by the servo. When
the speed reaches the limit amplitude, it will enter the speed regulation stage. The commonly used object dictionaries for
using PT mode are as follows:
            Index      Subindex                       Name                  type       data type                 unit

            603Fh         00                     Error code                  RO           UINT16                      -
            6041h         00                    Status word                  RO           UINT16                      -
            6061h         00                   Mode display                  RO            INT8                       -
            606Ch         00                   Actual speed                  RO           INT32            Command Unit /S
            6074h         00                 Torque command                  RO           INT16                  0.1%
            6077h         00                   Actual torque                 RO           INT16                  0.1%
            6040h         00                   Control word                 RW            UINT16                      -
            6060h         00                   control model                RW             INT8                       -
            6071h         00                   Target torque                RW            INT16                  0.1%
            6087h         00                   Torque slope                 RW        UDINT32                   0.1%/S
            607Fh         00                 Maximum speed                  RW        UDINT32              Command Unit /S
            6072h         00                 Maximum torque                 RW            UINT16                 0.1%
            60B2h         00                   Torque offset                RW            INT32                  0.1%
            60E0h         00               Forward torque limit             RW            UINT16                 0.1%
            60E1h         00               Reverse torque limit             RW            UINT16                 0.1%




    -28-


---

<!-- PDF page 33 -->

EtherCAT 总线伺服驱动器用户手册

                                            Servo ON


                                       Torque slope 6087h
                                       Target torque 6071h



                                         Polarity 607Eh


                                        Maximum speed
                                             limit
                                            607Fh                                  Speed feedback
                                                                                       606Ch
                                            +       -               Speed regulator
                                                                       2008-01h
                                                                       2008-02h

                                    Torque filtering 2007-06h
                                     Maximum torque 6072h                Motor    Encoder


                                                       Torque feedback
                                                           6077h

                                                  PT Control mode diagram

7.9 Return to Origin Mode(Home mode)
      Set 6060h to 6 and put the drive in HOME mode. The origin return to zero mode is used to find the mechanical
origin and locate the position relationship between the mechanical origin and the mechanical zero point. Mechanical
origin: A fixed position on the machine that can correspond to a specific origin switch and the motor Z signal. Mechanical
zero point: the absolute zero position on the machine. After returning to zero, the motor stops at the mechanical origin. By
setting 607Ch, the relationship between the mechanical origin and the mechanical zero point can be set: mechanical
origin=mechanical zero point+607Ch (origin offset). When 607Ch=0, the mechanical origin coincides with the
mechanical zero point. Zero return method 6098h, please refer to the appendix. The commonly used object dictionaries
for using the hm mode are as follows:

            Index      Subindex                 Name                       type       data type           unit
            603Fh         00                  Error code                   RO         UINT16               -
            6041h         00                 Status word                   RO         UINT16               -
            6061h         00                Mode display                   RO          INT8                -
            6064h         00              Position feedback                RO          INT32         Command Unit
            606Ch         00                 Actual speed                  RO          INT32        Command Unit /S
            6077h         00                Actual torque                  RO          INT16             0.1%
            60FDh         00                Input IO status                RO         UINT32               -
            60F4h         00              Position deviation               RO         DINT32         Command Unit
            6040h         00                Control word                   RW         UINT16               -
            6060h         00                control model                  RW          INT8                -
            6098h         00                Home method                    RW          INT8                -
                          01              Home high speed                  RW         UINT32        Command Unit /S
            6099h
                          02               Home low speed                  RW         UINT32        Command Unit /S
            609Ah         00              Home acceleration                RW         UDINT32       Command Unit /S2
            2005h         24             Home Timeout time                 RW         UINT16             10ms


                                                                                                                        -29-


---

<!-- PDF page 34 -->

EtherCAT 总线伺服驱动器用户手册
                                        Excessive position deviation
            6065h          00                                                   RW          UINT32               Command Unit
                                              alarm threshold
            6067h          00              Position reaches threshold           RW          UINT32                    Encoder unit
            6068h          00              Position reaches window              RW          UINT16                        ms

                                     Servo ON



                               Home speed 6099h
                              Home method 6098h
                            Home acceleration 609Ah
                              Home offset 607Ch
                           Home timeout time 2005-24h


                                     Gear ratio
                                     6091-01h
                                     6091-02h                                       Torque feedback
                                                                                        6077h
                                             Position feedback
                                       +           6063h
                                                            Encoder     Motor
                                                                                            Torque filtering 2007-06h

               Speed feedforward
                                              -                                                 Torque limitation
                                                                                                                           + +
                   2008-14h                                         Speed feedback
                 Speed filtering    Position gain          speed        606Ch                      Speed regulator
                   2008-13h          2008-03h             calculate
                                                                             -+                  2008-01h, 2008-02h

                                       +
                                   +                   Maximum speed limit               Torque feedforward gain 2008-16h
                                                             607Fh                      Torque feedforward filtering 2008-15h


                                                    HOME Control mode diagram


7.10 Probe Function Description
     The probe function refers to the position locking function. It can lock the position information (instruction unit) when
the external DI signal or motor Z signal changes. This series of servos supports the simultaneous activation of two probes,
and can simultaneously record the position information corresponding to the rising and falling edges of each probe signal,
allowing for the simultaneous locking of four position information. Probe 1 can choose DI3 or motor Z signal as the probe
signal, while Probe 2 can choose DI4 or motor Z signal as the probe signal. The relevant parameters for using the probe
function are as follows:
                                                                           default          Set
    Index     Subindex                        Name                                                     Type      Data type               Unit
                                                                            value          value
    2003h        09                DI3 function selection                       0           38         RW         UINT16                  -
    2003h        0A                 DI3 logic selection                         0            2         RW         UINT16                  -
    2003h        0B                DI4 function selection                       0           39         RW         UINT16                  -
    2003h        0C                 DI4 logic selection                         0            2         RW         UINT16                  -
    60B8         00                    Probe function                           0          4883        RW         UINT16                  -
    60B9         00                        Probe status                         0            -         RO         UINT16                  -
    60BA         00        Probe 1 rising edge latch position                   0            -         RO         INT32              command unit
    60BB         00        Probe 1 falling edge latch position                  0            -         RO         INT32              command unit
    60BC         00        Probe 2 rising edge latch position                   0            -         RO         INT32              command unit
    60BD         00        Probe 2 falling edge latch position                  0            -         RO         INT32              command unit




    -30-


---

<!-- PDF page 35 -->

EtherCAT 总线伺服驱动器用户手册

7.10.1 Function Description of 60B8h and 60B9h
     Index   Subindex                                        Function Description
                        Bit0           0: Probe 1 is disabled; 1: Probe 1 is enabled;
                        Bit1           0: Probe 1 single mode; 1: Probe 1 continuous mode;
                        Bit2           Probe 1 trigger signal selection: 0: DI3; 1: Z signal;
                        Bit3           reserve
                                       0: The rising edge of probe 1 is not enabled; 1: The rising edge of probe 1 is
                        Bit4
                                       enabled;
                                       0: The falling edge of probe 1 is not enabled; 1: The falling edge of probe 1 is
                        Bit5
                                       enabled;
                        Bit6～ Bit7     reserve;
     60B8h     00h
                        Bit8           0: Probe 2 is not enabled; 1: Probe 2 is enabled;
                        Bit9           0: Probe 2 single mode; 1: Probe 2 continuous mode;
                        Bit10          Probe 2 trigger signal selection: 0: DI4; 1: Z signal;
                        Bit11          reserve;
                                       0: The rising edge of probe 2 is not enabled; 1: The rising edge of probe 2 is
                        Bit12
                                       enabled;
                                       0: The falling edge of probe 2 is not enabled; 1: The falling edge of probe 2 is
                        Bit13
                                       enabled;
                        Bit14～ Bit15   reserve;
                        Bit0           0: Probe 1 is not in action; 1: Probe 1 is working;
                                       0: The rising edge capture of probe 1 is not completed; 1: The rising edge capture
                        Bit1
                                       of probe 1 is completed;
                                       0: Probe 1 falling edge capture is not completed; 1: Probe 1 falling edge capture
                        Bit2
                                       is completed;
                        Bit3～Bit5      reserve;
                        Bit6           Probe 1 trigger signal selection: 0: DI3; 1: Z signal;
                        Bit7           Probe 1 trigger signal monitoring: 0: DI3 low level; 1: DI3 high level;
     60B9h     00h
                        Bit8           0: Probe 2 is not in action; 1: Probe 2 is working;
                                       0: Probe 2 rising edge capture is not completed; 1: Probe 2 rising edge capture is
                        Bit9
                                       completed;
                                       0: Probe 2 falling edge capture is not completed; 1: Probe 2 falling edge capture
                        Bit10
                                       is completed;
                        Bit11～Bit13    reserve;
                        Bit14          Probe 2 trigger signal selection: 0: DI4; 1: Z signal;
                        Bit15          Probe 2 trigger signal monitoring: 0: DI4 low level; 1: DI4 high level;




                                                                                                                            -31-


---

<!-- PDF page 36 -->

EtherCAT 总线伺服驱动器用户手册

7.10.2 Read the probe latch position

     The four position information of the probe are recorded in objects 0x60BA to 0x60BD, as shown in the following
figure. The rising edge position latch function of probe 1 has been executed, and the position information can be read by
reading 0x60BA (feedback latch value of rising edge position of probe 1, instruction unit).
     The working mode of a single probe is as follows:



                                     60B8h
                                 Bit0/Bit8

                                     60B8h
                     Bit4/Bit5/Bit12/Bit13


                                     60B9h
                                 Bit0/Bit8



                                     60B9h
                      Bit1/Bit2/Bit9/Bit10




                              60BAh~ 60BDh
                                                              XXX                       YYY




                              Input Signal



    The working mode of the continuous probe is as follows:



                                     60B8h
                                 Bit0/Bit8

                                   60B8h
                   Bit4/Bit5/Bit12/Bit13


                                     60B9h
                                 Bit0/Bit8


                                   60B9h
                    Bit1/Bit2/Bit9/Bit10



                             60BAh~ 60BDh
                                                                XXX     YYY     ZZZ




                             60D5h~ 60D8h
                                                                    1    2       3




                             Input Signal




    -32-


---

<!-- PDF page 37 -->

EtherCAT 总线伺服驱动器用户手册

                          Chapter VIII Parameter Description
8.1 1000h object dictionary
Index   Subindex                    Name                    Unit   Change method           Describe        attribute
1000      00       Equipment type                            -     Unable modify   CIA standard              RO
1001      00       error register                            -     Unable modify   CIA error register        RO
1008      00       Manufacturer equipment name               -     Unable modify   -                         RO
1009      00       Manufacturer hardware version             -     Unable modify   -                         RO
100A      00       Manufacturer software version             -     Unable modify   -                         RO
          00       Number of sub-indexes                     -     Unable modify   -                         RO
          01       Vendor ID                                 -     Unable modify   -                         RO
1018      02       Product Code                              -     Unable modify   -                         RO
          03       Modify encoding                           -     Unable modify   -                         RO
          04       serial number                             -     Unable modify   -                         RO
                   Synchronization management
          00       communication type maximum sub-index      -     Unable modify   -                         RO
                   number
1C00      01       SM0 communication type                    -     Unable modify   -                         RO
          02       SM1 communication type                    -     Unable modify   -                         RO
          03       SM2 communication type                    -     Unable modify   -                         RO
          04       SM3 communication type                    -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Live changes    -                         RW
                   RPDO1
1600
                                                                                   Default RxPDO mapping
        01～0A      RxPDO mapping object group 1              -     Live changes                              RW
                                                                                   group 1
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1701               RPDO258
        01～04      mapping object                            -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1702               RPDO259
        01～07      mapping object                            -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1703               RPDO260
        01～07      mapping object                            -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1704               RPDO261
        01～09      mapping object                            -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1705               RPDO262
        01～08      mapping object                                  Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Live changes    -                         RW
                   TPDO1
1A00
                                                                                   Default TxPDO mapping
        01～0A      TxPDO mapping object group 1              -     Live changes                              RW
                                                                                   group 1
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1B01               TPDO258
        01～08      mapping object                            -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1B02               TPDO259
        01～09      mapping object                            -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1B03               TPDO260
        01～0A      mapping object                            -     Unable modify   -                         RO
                   Number of mapping objects supported by
          00                                                 -     Unable modify   -                         RO
1B04               TPDO261
        01～0A      mapping object                            -     Unable modify   -                         RO
1C12    00～01      RxPDO allocation                          -     Live changes    -                         RW
1C13    00～01      TxPDO allocation                          -     Live changes    -                         RW
1C32    00～0A      RxPDO management parameters               -     Live changes    -                         RO
1C33    00～0A      TxPDO management parameters               -     Live changes    -                         RO
                                                                                                                  -33-


---

<!-- PDF page 38 -->

EtherCAT 总线伺服驱动器用户手册

8.2 2000h Object Dictionary Parameters

8.2.1 2001 Group object dictionary (P01 group parameter)
Function      Sub                                                                                       Defau       Setting       attrib
  code Index index             Name                                Set Range                     unit     lt       effective       ute type
                                                                                                                  Stop setting
P01.02    2001   03   Servo drive number                           0～65535                        -       -     Restart effective RW Uint16
                      Software version
P01.50    2001   32   number                                            -                         -       -            -         RO Uint16


8.2.2 2002 Group Object Dictionary (P02 Group Parameters)
Function      Sub                                                                                       Defau       Setting       attrib
  code Index index             Name                                Set Range                     unit     lt       effective       ute type
                                                                                                                  Stop setting
P02.00 2002      01   Control mode selection 9：EtherCAT Mode                                      -       9        Effective       RO Uint16
                                                                                                                 immediately
P02.01 2002      02   Encoder type selection 0: Incremental encoder                               -       0       Stop setting     RW Uint16
                                             1: Absolute encoder                                                Restart effective
                                             0: Take the CCW direction as the forward
P02.02 2002      03   Rotation direction     direction                                            -       0       Stop setting    RW Uint16
                      selection              1: Take the CW direction as the forward direction                  Restart effective
                                             0: Take the CCW direction as the forward
                                             direction (A leads B)                                                Stop setting
P02.03 2002      04   Output pulse phase     1: Take the CW direction as the forward              -       0     Restart effective RW Uint16
                                             direction (A lags B)
                      Servo enable OFF       0: Coast to stop, maintain free running state                       Stop setting
P02.05 2002      06   shutdown mode          1: Stop at zero speed and maintain free running      -       0        Effective     RW Uint16
                      selection              state                                                               immediately
                                             0: Coast to stop, maintain free running state
                                             1: Stop at zero speed, the position remains                         Stop setting
P02.07 2002      08   Overtravel stop mode locked                                                 -       1        Effective     RW Uint16
                      selection              2: Stop at zero speed and maintain free running                     immediately
                                             state
                                                                                                                 Stop setting
P02.08 2002      09   Fault No.1 shutdown       0: Coast to stop, maintain free running state     -       0        Effective    RW Uint16
                      mode selection                                                                             immediately
                    Delay from brake output                                                                     Running setting
P02.09 2002      0A ON to command           0～500                                                ms     250        Effective    RW Uint16
                    reception                                                                                    immediately
                    In static state, delay                                                                      Running setting
P02.10 2002      0B from brake output OFF 1～1000                                                 ms     150        Effective    RW Uint16
                    to motor                                                                                     immediately
                    de-energization
                    Rotating state, speed                                                                       Running setting
P02.11 2002      0C threshold when brake 0～3000                                                  rpm     30        Effective    RW Uint16
                    output is OFF                                                                                immediately
                    Rotating state, delay                                                                       Running setting
P02.12 2002      0D from servo enable OFF 1～1000                                                 ms     500        Effective    RW Uint16
                    to brake output OFF                                                                          immediately
                      LED warning display       0: Output warning information immediately                        Stop setting
P02.15 2002      10   selection                 1: No warning message is output                   -       0        Effective    RW Uint16
                                                                                                                 immediately
                      The minimum value of
P02.21 2002      16   the braking resistor      -                                                 Ω      40            -         RO Uint16
                      allowed by the driver
                      Built-in braking resistor
P02.22 2002      17   power                     -                                                W       50            -         RO Uint16
                      Built-in braking resistor
P02.23 2002      18   resistance                -                                                 Ω      50            -         RO Uint16

                      Resistor heat                                                                              Stop setting
P02.24 2002      19   dissipation coefficient   10～100                                           %       30        Effective     RW Uint16
                                                                                                                 immediately
                                              0: Use built-in braking resistor
                                              1: Use external braking resistor and natural
                                              cooling                                                            Stop setting
P02.25 2002      1A Braking resistor settings 2: Use external braking resistor and forced air     -       0        Effective     RW Uint16
                                              cooling                                                            immediately
                                              3: No braking resistor is needed, all depends on
                                              capacitor absorption.
                                                                                                                 Stop setting
P02.26 2002      1B   External braking          1～65535                                          W        -        Effective     RW Uint16
                      resistor power                                                                             immediately

   -34-


---

<!-- PDF page 39 -->

EtherCAT 总线伺服驱动器用户手册
                       External braking                                                                          Stop setting
P02.27 2002     1C     resistor resistance     1～1000                                            Ω        -        Effective      RW Uint16
                                                                                                                 immediately
                                               0: No operation                                                   Stop setting
P02.31 2002     20     System parameter        1: Restore factory values (except P00/P01          -      0         Effective      RW Uint16
                       initialization          group)                                                            immediately
                                               2: Clear fault records
                                               0~99, corresponding to the PB group parameter                    Running setting
P02.32 2002     21     Panel default display   number, setting bit 0 corresponds to speed         -      50        Effective    RW Uint16
                       function                monitoring, setting bit 13 corresponds to pulse                   immediately
                                               monitoring

8.2.3 2003 Group Object Dictionary (P03 Group Parameters)
Function        Sub                                                                                     Defau       Setting       attri
         Index                  Name                               Set Range                     unit                                   type
  code         index                                                                                     lt        effective      bute
                                               Set the hexadecimal encoding (0000 to FFFF)
                       Effective DI function corresponding to the DI function (FunIN.1 to
                                                                                                                Running setting
P03.00   2003   01     allocation for power on FunIN.16).                                         -      0                        RW Uint16
                                                                                                                Restart effective
                       1                       After reconnecting the control power, the DI
                                               function becomes effective immediately.
                                               Set the hexadecimal encoding (0000 to FFFF)
                                               corresponding to the DI function (FunIN.17 to
                       Power on effective DI                                                                    Running setting
P03.01   2003   02                             FunIN.32).                                         -      0                        RW Uint16
                       function allocation 2                                                                    Restart effective
                                               After reconnecting the control power, the DI
                                               function becomes effective immediately.
                                                                                                                Running setting
                       DI0 terminal function
P03.02   2003   03                             0～39，Refer to Chapter 4.5.1                        -      14       No enable     RW Uint16
                       selection
                                                                                                                   effective
                                               Input polarity: 0-4
                                               0. Indicates that the low level is valid
                                               1. Indicates that the high level is valid                        Running setting
                       DI0 terminal logic
P03.03   2003   04                             2. Indicates that the rising edge is effective     -      0        No enable     RW Uint16
                       selection
                                               3. Indicates that the falling edge is effective                     effective
                                               4. Indicates that both the rising and falling
                                               edges are effective
                                                                                                                Running setting
                       DI1 terminal function
P03.04   2003   05                             0～39，Refer to Chapter 4.5.1                        -      15       No enable       RW Uint16
                       selection
                                                                                                                   effective
                                                                                                                Running setting
                       DI1 terminal logic
P03.05   2003   06                             Refer to P03.03 for instructions                   -      0        No enable       RW Uint16
                       selection
                                                                                                                   effective
                                                                                                                Running setting
                       DI2 terminal function
P03.06   2003   07                             0～39，Refer to Chapter 4.5.1                        -      31       No enable       RW Uint16
                       selection
                                                                                                                   effective
                                                                                                                Running setting
                       DI2 terminal logic
P03.07   2003   08                             Refer to P03.03 for instructions                   -      0        No enable       RW Uint16
                       selection
                                                                                                                   effective
                                                                                                                Running setting
                       DI3 terminal function
P03.08   2003   09                             0～39，Refer to Chapter 4.5.1                        -      0        No enable       RW Uint16
                       selection
                                                                                                                   effective
                                                                                                                Running setting
                       DI3 terminal logic
P03.09   2003   0A                             Refer to P03.03 for instructions                   -      0        No enable       RW Uint16
                       selection
                                                                                                                   effective
                                                                                                                Running setting
                       DI4 terminal function
P03.10   2003   0B                             0～39，Refer to Chapter 4.5.1                        -      0        No enable       RW Uint16
                       selection
                                                                                                                   effective
                                                                                                                Running setting
                       DI4 terminal logic
P03.11   2003   0C                             Refer to P03.03 for instructions                   -      0        No enable       RW Uint16
                       selection
                                                                                                                   effective
                                               Set the hexadecimal encoding (0000~FFFF)
                       Effective DI function corresponding to the DI function
                                                                                                                Running setting
P03.34   2003   23     allocation for power on (FunIN.33~FunIN.48).                               -      0                        RW Uint16
                                                                                                                Restart effective
                       3                       After reconnecting the control power, the DI
                                               function becomes effective immediately.

8.2.4 2004 Group Object Dictionary (P04 Group Parameters)
Function        Sub                                                                                     Defau       Setting       attrib
         Index                  Name                               Set Range                     unit                                    type
  code         index                                                                                      lt       effective       ute
                       DO0terminal function                                                                        Running
P04.00   2004   01                             0～20，Refer to Chapter 5.4.2                        -       1                       RW Uint16
                       selection                                                                                    setting
                                                                                                                                          -35-


---

<!-- PDF page 40 -->

EtherCAT 总线伺服驱动器用户手册
                                                                                                                   No enable
                                                                                                                   effective
                                               Output polarity reversal setting: 0～1
                                                                                                                    Running
                                               0. Indicates that the output is low level when it
                                                                                                                     setting
P04.01    2004   02 DO0terminal logic          is valid (the optocoupler is turned on)              -      0                      RW Uint16
                                                                                                                    No enable
                    selection                  1. Indicates that the output is high level when
                                                                                                                    effective
                                               valid (optocoupler is turned off)
                                                                                                                     Running
                    DO1terminal function                                                                              setting
P04.02    2004   03                            0～20，Refer to Chapter 5.4.2                          -      5                      RW Uint16
                    selection                                                                                       No enable
                                                                                                                     effective
                                                                                                                     Running
                      DO1terminal logic                                                                               setting
P04.03    2004   04                            Refer to P04.01 instructions                         -      0                      RW Uint16
                      selection                                                                                     No enable
                                                                                                                     effective
                                                                                                                     Running
                      DO2 terminal function                                                                           setting
P04.04    2004   05                            0～20，Refer to Chapter 5.4.2                          -      3                      RW Uint16
                      selection                                                                                     No enable
                                                                                                                     effective
                                                                                                                     Running
                      DO2 terminal logic                                                                              setting
P04.05    2004   06                            Refer to P04.01 instructions                         -      0                      RW Uint16
                      selection                                                                                     No enable
                                                                                                                     effective
                                               Set whether the DO function logic selected by                         Running
                                               the hardware DO terminals (DO1~DO3) is                                 setting
P04.22    2004   17                                                                                 -      0                      RW Uint16
                      DO source selection      determined by the actual status of the drive or                      Effective
                                               communication settings.                                             immediately

8.2.5 2005 Group Object Dictionary (P05 Group Parameters)
Function      Sub                                                                                         Defau       Setting     attrib
  code Index index             Name                                Set Range                       unit     lt       effective     ute type
                                                                                                                   Stop setting
P05.04    2005   05   First-order low-pass     0～6553.5                                            ms      0.0      Effective     RW Uint1
                      filter time constant                                                                         immediately          6
                                                                                                                   Stop setting
P05.06    2005   07   Average filter time      0.0～128.0                                           ms      0.0      Effective     RW Uint1
                      constant                                                                                     immediately          6
                                                                                                                   Stop setting
P05.16    2005   11   Clear position deviation 0～2                                                  -       0       Effective     RW Uint1
                      action selection                                                                             immediately          6

                      Encoder frequency        Set the number of pulses for one revolution of      Enc         Stop setting       Uint1
P05.17    2005   12   division pulse number    the motor.                                          oder 2500 Restart effective RW   6
                                                                                                   unit
                                               0. No speed feedforward                                         Stop setting
P05.19    2005   14   Speed feed forward       1. Internal speed feed forward                       -    1      Effective      RW Uint1
                      control selection        2. Use 60B1 as speed feed forward                              immediately           6

                                               Set the default motor direction, deceleration
                                               point, and origin when returning to zero.
                                                        direction   Referenc      origin
                                                                    e point
                                                  0 forward         origin        origin
                                                  1 reverse         origin        origin
                                                  2 forward         Z signal      Z signal
                                                  3 reverse         Z signal      Z signal
                                                                                                   Enc             Stop setting
P05.31    2005   20   Return to origin mode       4 forward         origin        Z signal         oder     0       Effective     RW Uint1
                                                  5 reverse         origin        Z signal         unit            immediately          6
                                                                    Positive      Positive
                                                  6 forward         limit         limit
                                                                    Negative      Negative
                                                  7 reverse         limit         limit
                                                                    Positive
                                                  8 forward         limit         Z signal

                                                  9 reverse         Negative      Z signal
                                                                    limit
                                                                                                                  Running setting
P05.35    2005   24   Limit the time to find   0～65535                                             10m 5000          Effective    RW Uint1
                      the origin                                                                    s              immediately         6
                                                                                                   Enc             Stop setting
P05.44    2005   2D Encoder multi-turn data 0～65535                                                oder     0        Effective    RW Uint1
                    offset                                                                         unit            immediately         6
                      absolute position linear                                                     Enc             Stop setting
P05.46    2005   2F   mode positionBias low -2147483648～2147483647                                 oder     0        Effective    RW Int32
                      32-bit                                                                       unit            immediately
   -36-


---

<!-- PDF page 41 -->

EtherCAT 总线伺服驱动器用户手册
                absolute position linear                                                     Enc              Stop setting
 P05.48 2005 31 mode positionBias low -2147483648～2147483647                                 oder     0         Effective      RW Int32
                32-bit                                                                       unit             immediately
                   Position arrival         0. Encoder unit                                                   Stop setting          Uint1
P05.61   2005   3E threshold unit selection 1. Command Unit                                   -       1         Effective      RW     6
                                                                                                              immediately

8.2.6 2006 Group Object Dictionary (P06 Group Parameters)
Function      Sub                                                                                   Defau     Setting    attrib
  code Index index           Name                             Set Range                      unit     lt     effective    ute type
                                                                                                         Running setting        Uint1
P06.04   2006   05 Jog speed setting value 0～6000                                            rpm     100 Effective        RW      6
                                                                                                         immediately
                   Torque feedforward      1. Internal torque feedforward                                Running setting        Uint1
P06.11   2006   0C control selection       2. Use 60B2 as external torque feedforward         -       1 Effective         RW      6
                                                                                                         immediately
                                           0～6000；When the speed command                                     Running setting
P06.15   2006   10 Zero position fixed     amplitude is less than or equal to the 2006-10h   rpm     10      Effective         RW Uint1
                   speed threshold         setting value, the servo motor enters Zero                        immediately              6
                                           position locked state

8.2.7 2007 Group Object Dictionary (P07 Group torque control parameters)
Function     Subin                                                                                  Defaul    Setting     attrib
  code Index dex             Name                             Set Range                      unit     t      effective     ute type
                                                                                                          Running setting
P07.05   2007   06 Torque command filter 0～30.00                                             ms      0.79   Effective      RW Uint16
                   time constant                                                                           immediately
                                                                                                          Running setting
P07.06   2007   07 Second torque command 0～30.00                                             ms      0.79   Effective      RW Uint16
                   filter time constant                                                                    immediately
                                          0. Positive and negative internal torque limits
                                          1. Positive and negative external torque limits
                                          2. EtherCAT positive and negative external
                                          torque limits
                                          3. The minimum value of the positive and
                                          negative external torque and the positive and                      Running setting
P07.07   2007   08 Torque limit source    negative external torque limit of ETherCAT is       -       2        Effective     RW Uint16
                                          the torque limit (P-CL, N-CL)                                       immediately
                                          4. Positive and negative internal torque and
                                          ETherCAT positive and negative external
                                          torqueSwitching between partial torque limit
                                          (P-CL, N-CL)
                   Positive internal torque                                                                  Running setting
P07.09   2007   0A limit                    0.0～300.0                                         %     300.0      Effective       RW Uint16
                                                                                                              immediately
                   Negative internal torque                                                                  Running setting
P07.10   2007   0B limit                    0.0～300.0                                         %     300.0      Effective       RW Uint16
                                                                                                              immediately
                   Positive external torque                                                                  Running setting
P07.11   2007   0C limit                    0.0～300.0                                         %     300.0      Effective       RW Uint16
                                                                                                              immediately
                   Negative external torque                                                                  Running setting
P07.12   2007   0D limit                    0.0～300.0                                         %     300.0      Effective       RW Uint16
                                                                                                              immediately
                                                                                                             Running setting
P07.15   2007   10 emergency stop torque 0.0～300.0                                            %     300.0      Effective       RW Uint16
                                                                                                              immediately
                                          0: Internal speed limit
                   Speed limit source     1: EtherCAT external speed limit                                   Running setting
P07.17   2007   12 selection              2: Select via FunIN.36                              -       0        Effective     RW Uint16
                                          2007-14h/2007-15h as internal speed limit                           immediately
                   Torque control forward                                                                    Running setting
P07.19   2007   14 speed limit / Torque   0～6000                                             rpm 3000          Effective     RW Uint16
                   control speed limit 1                                                                      immediately
                   Torque control negative                                                                   Running setting
P07.20   2007   15 speed limit / Torque    0～6000                                            rpm 3000          Effective     RW Uint16
                   control speed limit 2                                                                      immediately

                   Torque reaches                                                                         Running setting
P07.21   2007   16 reference value        0.0～300.0                                           %      0.0    Effective     RW Uint16
                                                                                                           immediately
                   Torque reaches effective                                                               Running setting
P07.22   2007   17 value                    0.0～300.0                                         %      20.0   Effective     RW Uint16
                                                                                                           immediately

                                                                                                                                     -37-


---

<!-- PDF page 42 -->

EtherCAT 总线伺服驱动器用户手册
                     Torque reaches invalid                                                                      Running setting
P07.23    2007    18 value                  0.0～300.0                                                %    10.0     Effective     RW Uint16
                                                                                                                  immediately
                     Speed limited window in                                                                     Running setting
P07.40    2007    29 torque mode             0.5～30.0                                                ms    1.0     Effective     RW Uint16
                                                                                                                  immediately

8.2.8 2008 Group Object Dictionary (P08 Group gain class parameters)
Function      Sub                                                                                         Defau                  attri
  code Index index             Name                                Set Range                     unit       lt Setting effective bute type
                                                                                                                Running setting
P08.00    2008   01   Speed loop gain          0.1～2000.0                                        Hz       25.0 Effective         RW Uint16
                                                                                                                immediately
                      Speed loop integration                                                                    Running setting
P08.01    2008   02   time constant            0.15～512.00                                       ms       31.83 Effective        RW Uint16
                                                                                                                immediately
                                                                                                                Running setting
P08.02    2008   03   Position loop gain       0.0～2000.0                                        Hz       40.0 Effective         RW Uint16
                                                                                                                immediately
                                                                                                                Running setting
P08.03    2008   04   2nd speed loop gain      0.1～2000.0                                        Hz       40.0 Effective         RW Uint16
                                                                                                                immediately
                      2nd speed loop                                                                            Running setting
P08.04    2008   05   integration time         0.15～512.00                                       ms       40.00 Effective        RW Uint16
                      constant                                                                                  immediately
                                                                                                                Running setting
P08.05    2008   06   2nd position loop gain   0.0～2000.0                                        Hz       64.0 Effective         RW Uint16
                                                                                                                immediately
                                               0. The first gain is fixed, use external DI for                   Running setting
P08.08    2008   09   2nd gain mode setting    P/PI switching                                    -        1      Effective       RW Uint16
                                               1. Use gain switching according to the                            immediately
                                               condition setting of P08.09

                                               0.First gain fixed (PS)
                                               1.Use external DI switching (PS)
                                               2. Large torque command (PS)
                                               3. Large speed command (PS)
                                               4. Large change rate of speed command (PS)
                      Gain switching           5. Speed command high and low speed                               Running setting
P08.09    2008   0A   condition selection      threshold (PS)                                    -        0      Effective       RW Uint16
                                               6. Large position deviation (P)                                   immediately
                                               7.With position command (P)
                                               8. Positioning completed (P)
                                               9.High actual speed (P)
                                               10. With position command + actual speed (P)

                      Gain switching delay                                                                       Running setting
P08.10    2008   0B   time                     0.0～1000.0                                        ms       5.0    Effective         RW Uint16
                                                                                                                 immediately
                                                                                                                 Running setting
P08.11    2008   0C   Gain switching level     0～20000, Unit switching based on conditions -              50     Effective         RW Uint16
                                                                                                                 immediately
                                                                                                                 Running setting
P08.12    2008   0D   Gain switching time lag 0～20000, Unit switching based on conditions -               30     Effective         RW Uint16
                                                                                                                 immediately
                      Position gain switching                                                                    Running setting
P08.13    2008   0E   time                    0.0～1000.0                                         ms       3.0    Effective         RW Uint16
                                                                                                                 immediately
                                                                                                                 Running setting
P08.15    2008   10   Load inertia ratio       0.00～120.00                                       倍        1.00   Effective         RW Uint16
                                                                                                                 immediately
                      Speed feedforward filter                                                                   Running setting
P08.18    2008   13   time constant            0.00～64.00                                        ms       0.50   Effective         RW Uint16
                                                                                                                 immediately
                                                                                                                 Running setting
P08.19    2008   14   Speed feedforward gain 0.0～100.0                                           %        0.0    Effective         RW Uint16
                                                                                                                 immediately
                                                                                                                 Running setting
P08.20    2008   15   Torque feedforward       0.00～64.00                                        ms       0.50   Effective         RW Uint16
                      filter time constant                                                                       immediately
                                                                                                                 Running setting
P08.21    2008   16   Torque feedforward       0.0～200.0                                         %        0.0    Effective         RW Uint16
                      gain                                                                                       immediately



   -38-


---

<!-- PDF page 43 -->

EtherCAT 总线伺服驱动器用户手册
                                              0. Disable speed feedback average filtering
                                              1. Speed feedback 2 times average filtering                      Stop setting
P08.22   2008   17   Speed feedback filtering 2. Speed feedback 4 times average filtering         -      0     Effective        RW Uint16
                     options                  3. Speed feedback 8 times average filtering                      immediately
                                              4.Speed feedback 16 times average filtering
                     Speed feedback                                                                            Running setting
P08.23   2008   18   low-pass filter cutoff   100～4000                                            Hz     4000 Effective        RW Uint16
                     frequency                                                                                 immediately
                      Pseudo-differential                                                                      Running setting
P08.24   2008   19    feedforward control     0.0～100.0                                           -      100.0 Effective       RW Uint16
                      coefficient                                                                              immediately

8.2.9 2009 Group Object Dictionary (P09 Group self adjustment parameters)
Function      Sub                                                                                        Defau                  attri
  code Index index             Name                               Set Range                       unit     lt Setting effective bute type
                                              0. Parameter self-adjustment is invalid,
                                              manually adjust the gain parameters;
                                              1. Parameter self-adjusting mode, using the
                                              rigidity table to automatically adjust the gain
                     Self-adjusting mode      parameters;                                                      Running setting
P09.00   2009 01     selection                2. Positioning mode, use the rigidity table to  -          0     Effective       RW Uint16
                                              automatically adjust the gain parameters;                        immediately
                                              3. Parameter self-adjusting mode with friction
                                              compensation;
                                              4. Positioning mode with friction compensation
                                                                                                               Running setting
P09.01   2009 02     Group 1 rigidity level   0～31                                                -      12    Effective       RW Uint16
                     selection                                                                                 immediately
                                              0. The adaptive notch filter is no longer updated
                                              1. Adaptive notches are effective (the third
                                              group of notches)
                     Adaptive notch mode      2. 2 adaptive notches are effective (3rd and 4th                 Running setting
P09.02   2009 03     selection                set of notches)                                   -        0     Effective       RW Uint16
                                              3. Only test the resonance point, which is                       immediately
                                              displayed in P09.24
                                              4. Restore the values of the 3rd and 4th groups
                                              of notches to factory settings
                                              0. Turn off online inertia identification
                                              1. Turn on online inertia identification and
                                              change slowly
P09.03   2009 04     Online inertia           2. Turn on online inertia identification, general                                 RW Uint16
                     identification mode      changes
                                              3.Turn on online inertia identification and
                                              change quickly
                     Low frequency                                                                             Running setting
P09.04   2009 05     resonance suppression    0. Manually set vibration frequency                 -      0     Effective       RW Uint16
                     mode selection           1. Automatically identify vibration frequency                    immediately
                     Offline inertia                                                                           Stop setting
P09.05   2009 06     identification mode      0. Positive and negative triangle wave mode         -      0     Effective       RW Uint16
                     selection                1.JOG jog mode                                                   immediately
                                                                                                               Stop setting
P09.06   2009 07     Inertia identification   100～1000                                            rpm 500      Effective       RW Uint16
                     maximum speed                                                                             immediately
                     Accelerate to the
                     maximum during inertia                                                                    Stop setting
P09.07   2009 08     identification          20～800                                               ms     125   Effective        RW Uint16
                     speed time constant                                                                       immediately
                     Waiting time after                                                                        Stop setting
P09.08   2009 09     completion of single    50～10000                                             ms     800   Effective        RW Uint16
                     inertia identification                                                                    immediately
                     Complete single inertia
P09.09   2009 0A     identification of motor 0.00～2.00                                            r      -     -                RO Uint16
                     Number of turns
                     Group 1 notch                                                                            Running setting
P09.12   2009 0D     frequency                50～4000                                             Hz     4000 Effective         RW Uint16
                                                                                                              immediately
                     Group 1 Notch Width                                                                      Running setting
P09.13   2009 0E     Class                    0～20                                                -      2    Effective         RW Uint16
                                                                                                              immediately
                     Group 1 Notch Depth                                                                      Running setting
P09.14   2009 0F     Rating                   0～99                                                -      0    Effective         RW Uint16
                                                                                                              immediately
                     Group 2 notch                                                                            Running setting
P09.15   2009 10     frequency                50～4000                                             Hz     4000 Effective         RW Uint16
                                                                                                              immediately

                                                                                                                                      -39-


---

<!-- PDF page 44 -->

EtherCAT 总线伺服驱动器用户手册
                     Group 2 Notch Width                                                                     Running setting
P09.16    2009 11    Class                    0～20                                            -        2     Effective       RW Uint16
                                                                                                             immediately
                     Group 2 Notch Depth                                                                     Running setting
P09.17    2009 12    Rating                   0～99                                            -        0     Effective       RW Uint16
                                                                                                             immediately
                     Group 3 notch                                                                           Running setting
P09.18    2009 13    frequency                50～4000                                         Hz       4000 Effective        RW Uint16
                                                                                                             immediately
                     Group 3 Notch Width                                                                     Running setting
P09.19    2009 14    Class                    0～20                                            -        2     Effective       RW Uint16
                                                                                                             immediately
                     Group 3 Notch Depth                                                                     Running setting
P09.20    2009 15    Rating                   0～99                                            -        0     Effective       RW Uint16
                                                                                                             immediately
                     Group 4 notch                                                                           Running setting
P09.21    2009 16    frequency                50～4000                                         Hz       4000 Effective        RW Uint16
                                                                                                             immediately
                                                                                                             Running setting
P09.22    2009 17    Group 4 Notch Width      0～20                                            -        2     Effective       RW Uint16
                     Class                                                                                   immediately
                                                                                                             Running setting
P09.23    2009 18    Group 4 Notch Depth      0～99                                            -        0     Effective       RW Uint16
                     Rating                                                                                  immediately
                     Resonance frequency
P09.24    2009 19    identification results   0～2                                             Hz       0         -                  RO Uint16
                                                                                                             Running setting
P09.30    2009 1F    Torque disturbance       0.0～100.0                                       %        0.0   Effective              RW Uint16
                     compensation gain                                                                       immediately
                     Torque disturbance                                                                      Running setting
P09.31    2009 20    observer filter time     0.00～25.00                                      ms       0.50 Effective               RW Uint16
                     constant                                                                                immediately
                                                                                                             Running setting
P09.38    2009 27    low frequency            1.0～100.0                                       Hz       100.0 Effective              RW Uint16
                     resonance frequency                                                                     immediately
                     Low frequency                                                                           Running setting
P09.39    2009 28    resonance frequency      0～10                                            -        2     Effective              RW Uint16
                     filter setting                                                                          immediately

8.2.10 200A Group Object Dictionary (P0A Group Fault and Protection Parameters)
Functio       Sub                                                                                      Defau                attrib
n code Index index            Name                               Set Range                    unit      lt Setting effective ute type

                     Power input phase loss 0. Enable fault prohibition warning                                  Running setting
P0A.00 200A     01   protection selection   1. Enable faults and warnings                         -        0       Effective     RW Uint16
                                            2. Disable faults and warnings                                        immediately
                                             0. Disable absolute position limit                                      Stop setting
P0A.01 200A     02   Absolute position limit 1. Enable absolute position limit                    -        0          Effective     RW Uint16
                     settings                2. Enable absolute position limit after origin                          immediately
                                             return
                                                                                                                 Running setting
P0A.03 200A     04   Power-off save function 0. Do not perform power-off save                     -        0       Effective        RW Uint16
                     enable selection        1. Execute power-off save                                            immediately
                                                                                                                  Stop setting
P0A.04 200A     05   Motor overload           50～300                                              %        100     Effective        RW Uint16
                     protection gain                                                                              immediately
                                                                                                                 Running setting
P0A.08 200A     09   Overspeed fault          0：Max speed ×1.2；                               rpm          0       Effective        RW Uint16
                     threshold                1～10000：200A-09h～Max speed×1.2；                                     immediately
                     Overrun protection                                                                          Running setting
P0A.12 200A     0D   function enabled         0. No speed protection                              -        1       Effective        RW Uint16
                                              1. Turn on speed protection                                         immediately
                     Judgment of low                                                          Enco               Running setting
P0A.16 200A     11   frequency resonance    1～1000                                             der         5       Effective     RW Uint16
                     position deviation                                                       unit                immediately
                     threshold
                     Speed Feedback display                                                                          Stop setting
P0A.25 200A     1A   value filter time      0～5000                                                ms       50         Effective     RW Uint16
                     constant                                                                                        immediately
                     Motor overload shield    0. Open motor overload detection                                     Stop setting
P0A.26 200A     1B   enable                   1. Shield motor overload warning and fault          -        0        Effective      RW Uint16
                                              detection                                                           immediately
                     Speed DO filter time                                                                          Stop setting
P0A.27 200A     1C   constant                 0～5000                                              ms       10       Effective      RW Uint16
                                                                                                                  immediately
P0A.28 200A     1D   Quadrature encoder       0～255                                           25ns         30      Stop setting    RW Uint16
                     filter time constant                                                                        Restart effective
   -40-


---

<!-- PDF page 45 -->

EtherCAT 总线伺服驱动器用户手册
                Locked rotor                                                                                        Running setting
 P0A.32 200A 21 over-temperature       10～65535                                                         ms    200     Effective     RW Uint16
                protection time window                                                                               immediately
                Stalled rotor          0. Shield motor stalled rotation                                             Running setting
 P0A.33 200A 22 over-temperature       over-temperature protection detection                             -     1      Effective     RW Uint16
                protection enabled     1. Enable motor stall over-temperature                                        immediately
                                       protection detection
                       Encoder multi-turn       0. No shielding                                                       Stop setting
P0A.36 200A     25     overflow fault selection 1. Shield                                                -     0       Effective     RW Uint16
                                                                                                                      immediately

8.2.11 P0B Group monitoring parameters
     For specific parameters, please refer to Chapter 5.1.6

8.2.12 200C Group Object Dictionary (P0C Group communication parameters)
Function Index Sub                  Name                            Set Range                           unit Defau Setting effective attrib type
  code         index                                                                                          lt                      ute
                                                                                                                   Running setting
P0C.00 200C     01     drive address            1～247                                                    -     1       Effective      RW Uint16
                                                                                                                     immediately
                                                0.2400 Kbp/s
                                                1.4800 Kbp/s
                       Serial port baud rate    2.9600 Kbp/s                                                        Running setting
P0C.02 200C     03     setting                  3.19200 Kbp/s                                            -     5      Effective     RW Uint16
                                                4.38400 Kbp/s                                                        immediately
                                                5.57600 Kbp/s

                                          0. No parity, 2 end bits                                                  Running setting
P0C.03 200C     04     MODBUS data format 1. Even parity, 1 end bit                                      -     0      Effective     RW Uint16
                                          2. Odd parity, 1 end bit                                                   immediately
                                          3. No parity, 1 end bit
                                                For the master station whose station number is
                                                automatically assigned, the station number assigned
P0C.04 200C     05     Site name corrected      to the slave station when using EtherCAT                 -     -           -         RO Uint16
                                                communication is displayed.
                                                For a master station that cannot automatically assign                 Stop setting
P0C.05 200C     06     site alias               a station number, when using EtherCAT                    -     0       Effective     RW Uint16
                                                communication, set the slave station number through                   immediately
                                                this object.
                                                0. Do not save
                                                1. 2000h series object dictionary is written and
                       Whether the              stored in EEPROM after communication                                Running setting
P0C.13 200C     0E     communication write      2. 6000h series object dictionary is written and         -     0      Effective     RW Uint16
                       function code value is   stored in EEPROM after communication                                 immediately
                       updated to EEPROM        3. The object dictionary of 2000h series and
                                                6000h series is written and stored in EEPROM
                                                after communication.
                       EtherCAT sync                                                                                Running setting
P0C.35 200C     24     interrupt lost            4～20                                                   1ms    9      Effective     RW Uint16
                       Disallowed times                                                                              immediately
P0C.36 200C     25     Port0 port CRC check 0～65535                                                     W      0           -         RO Uint16
                       error
                       Port1 port CRC check
P0C.37 200C     26     error                     0～65535                                                W      0           -         RO Uint16
                       error
P0C.38 200C     27     Port 0, 1 data forwarding 0～65535                                                W      0           -         RO Uint16
                       error
P0C.39 200C     28     Processing unit and PDI 0～65535                                                  W      0           -         RO Uint16
                       errors
P0C.40 200C     29     Port 0, 1 link lost       0～65535                                                W      0           -         RO Uint16
                       Synchronization error                                                                          Stop setting
P0C.42 200C     2B     monitoring mode           0～1                                                     -     0       Effective     RW Uint16
                       setting                                                                                        immediately




                                                                                                                                            -41-


---

<!-- PDF page 46 -->

EtherCAT 总线伺服驱动器用户手册
                                                 0: The driver working sequence is
                                                 asynchronous with the host computer
                                                 synchronization clock. 1: Suitable for the host
                                                 computer synchronization performance
                                                 indicators to meet                                             Stop setting
P0C.43 200C     2C     Sync mode settings        In the case of 1us jitter (standard performance  -     2        Effective     RW Uint16
                                                 index of EtherCAT master station). 2: Suitable                 immediately
                                                 for host computer synchronization performance
                                                 indicators exceeding
                                                 In the case of 1us jitter (standard performance
                                                 indicator of EtherCAT master station)
                                                 0～2000: Used to set the jitter range of the
                   Synchronization error         synchronization signal allowed when the driver                 Stop setting
P0C.44 200C     2D threshold                     works in synchronization 1 mode                 1nm   500       Effective     RW Uint16
                                                 (200C-2Ch=1).                                                  immediately

                                               0: Disable location caching                                      Stop setting
P0C.45 200C     2E     Location cache settings 1: Enable location cache                          -      1        Effective     RW Uint16
                                                                                                                immediately
                       CSP position command 1～7; the counting threshold when the position                      Running setting
P0C.46 200C     2F     increment excessive  command increment exceeds the maximum                -      3        Effective     RW Uint16
                       threshold            position command increment                                          immediately
                       CSP position command 0~65535; the count value when the position
P0C.47 200C     30     increment too large  command increment exceeds the maximum                -      0            -         RO Uint16
                       times                position command increment threshold

8.2.13 P0D Group auxiliary function parameters
Function Index Sub              Name                                   Set Range                unit Defaul Setting effective attrib type
  code         index                                                                                   t                       ute
                                                 0. No operation                                              Stop setting
P0D.00 200D     01     software reset            1. Enable                                       -     0        Effective      RW Uint16
                                                                                                              immediately
                                                 0. No operation                                              Stop setting
P0D.01 200D     02     Fault reset               1. Enable                                       -     0        Effective      RW Uint16
                                                                                                              immediately
                       Offline inertia                                                                      Running setting
P0D.02 200D     03     identification function   -                                               -     -        Effective      RW Uint16
                                                                                                              immediately
P0D.03 200D     04     Initial angle recognition 1. Enable                                       -     -            -          RW Uint16
                                                                                                            Running setting
                                                 0. No operation
P0D.05 200D     06     Emergency shutdown
                                                 1. Enable emergency shutdown
                                                                                                 -     0        Effective      RW Uint16
                                                                                                              immediately
                       JOG trial operation
P0D.11 200D     0C     function                  (comes with filter)                             -       -           -         RW Uint16
                                             0. No operation
                                             1. Force DI to be enabled and force DO to be
                                             disabled.                                                         Running setting
                       DIDO forced input and 2. Force DO to enable, force DI to disable
P0D.17 200D     12     output enable
                                                                                                 -       0       Effective     RW Uint16
                                             3. Force DIDO to be enabled                                        immediately
                                             4. EtherCAT control forces DO to be enabled
                                             and forces DI to disable.
                                                                                                               Running setting
                                                                                                       0x01F
P0D.18 200D     13     DI forced input given     0～0x01FF                                        -
                                                                                                         F
                                                                                                                 Effective     RW Uint16
                                                                                                                immediately
                                                                                                               Running setting
P0D.19 200D     14     DO forces output given 0～0x001F                                           -       0       Effective     RW Uint16
                                                                                                                immediately

                       Absolute encoder reset 0. No operation                                                   Stop setting
P0D.20 200D     15     enable                 1. Reset fault                                     -       0       Effective     RW Uint16
                                              2. Reset fault and multi-turn data                                immediately




   -42-


---

<!-- PDF page 47 -->

EtherCAT 总线伺服驱动器用户手册

8.3 6000 Group Object Dictionary
         Sub                                                                                   Setting
Index   index          Name                       Set Range               unit      Default   effective    attribute    type

603F      00    error code            0～65535                               -         0           -          RO        Uint16
                                                                                              Running
6040     00     control word          0～65535                               -         0        setting       RW        Uint16
                                                                                              No enable
                                                                                              effective
6041     00     status word           0～xFFFF                               -         0           -          RO        Uint16
                Quick shutdown                                                                Running
                mode selection                                                                 setting
605A     00     choose                0～7；Refer to section Appendix 1       -         2       No enable      RW        INT16
                                                                                              effective
                                                                                              Running
                Temporary                                                                      setting
605D     00     shutdown method       1～3；Refer to section Appendix 1       -         1       No enable      RW        INT16
                selection                                                                     effective
                                                                                              Running
                Servo mode                                                                     setting
6060     00     selection             0～10；Refer to section 7.2.1           -         0       No enable      RW         INT8
                                                                                              effective
6061     00     Run mode display      0～10                                  -         0           -          RO         INT8

6062     00     position command      -                                 command        -          -          RO        Dint32
                                                                           unit
6063     00     position feedback     -                                  Encoder       -          -          RO        Dint32
                                                                           unit
                                                                        command
6064     00     position feedback     -                                    unit        -          -          RO        Dint32
                                                                                               Running
                Position deviation                                      command                 setting
6065     00     excessive threshold   0～2147483647                        unit      1048576   No enable      RW        UDint32
                                                                                               effective
                                                                                               Running
                                                                                                setting
6067     00     Position reaches      0～2147483647                      Encoder      734       Effective     RW        UINT32
                threshold                                                 unit                immediate
                                                                                                   ly
                                                                                               Running
                Location arrival                                                                setting
6068     00     window time           0～65535                             ms          16       Effective     RW        UINT16
                                                                                              immediate
                                                                                                   ly
606C     00     actual speed          -                                 command        -          -          RO        INT32
                                                                         unit /S
                                                                                              Running
606D     00     speed reaches         0～65535                             rpm         10       setting       RW        UINT16
                threshold                                                                     No enable
                                                                                              effective
                                                                                              Running
                Speed arrival                                                                  setting
606E     00     window time           0～65535                             ms          0       No enable      RW        UINT16
                                                                                              effective
                                                                                              Running
                                                                                               setting
6071     00     target torque         -4000～4000                         0.1%         0       No enable      RW        UINT16
                                                                                              effective
                                                                                              Running
                Maximum torque                                                                 setting
6072     00     command               0～4000                             0.1%        5000     No enable      RW        UINT16
                                                                                              effective
6074     00     Torque command        -5000～5000                         0.1%         0           -          RO        INT16

6077     00     actual torque         -5000～5000                         0.1%         0           -          RO        INT16
                                                                                              Running
607A     00     target location       -2147483648～2147483647            command       0        setting       RW        INT32
                                                                          unit                No enable
                                                                                              effective
                                                                                              Running
                                                                        command                setting
607C     00     Origin offset         -2147483648～2147483647              unit        0       No enable      RW        INT32
                                                                                              effective
                Minimum location                                                              Running
607D     01     limit                 -2147483648～2147483647            user unit    -231      setting       RW        INT32

                                                                                                                           -43-


---

<!-- PDF page 48 -->

EtherCAT 总线伺服驱动器用户手册
                                                                                                   No enable
                                                                                                    effective
                                                                                                    Running
607D     02   Maximum location       -2147483648～2147483647               user unit     231          setting  RW  INT32
              limit                                                                                No enable
                                                                                                    effective
                                     Set the polarity of position
                                     command, speed command and
                                     torque command.                                              Running
607E     00   Command polarity       Bit0~Bit4: Undefined; Bit5: Torque       -          0         setting     RW   UINT8
                                     command polarity; Bit6: Speed                                No enable
                                     command polarity; Bit7: Position                             effective
                                     command polarity; ON: Negate the
                                     command.
                                                                                                  Running
                                                                          command     10485760     setting
607F     00   Maximum speed          0～2147483647                          unit /S       0        No enable    RW   UINT32
                                                                                                  effective
                                                                                                  Running
6081     00   Contour running        0～2147483647                         user unit      0         setting     RW   UINT32
              speed                                                                               No enable
                                                                                                  effective
                                                                                                  Running
6083     00   Profile acceleration   0～2147483647                         command      100000      setting     RW   UINT32
                                                                           unit /S2               No enable
                                                                                                  effective
                                                                                                  Running
6084     00   Profile deceleration   0～2147483647                         command      100000      setting     RW   UINT32
                                                                           unit /S2               No enable
                                                                                                  effective
                                                                                                  Running
              Emergency stop                                              command                  setting
6085     00   deceleration           0～2147483647                          unit /S2    100000     No enable    RW   UINT32
                                                                                                  effective
              Operating curve
6086     00   selection              0- linear                                -          0            -        RW   INT16

                                                                                                   Running
                                                                                                    setting
6087     00   Torque ramp            0～2147483647                          0.1%/S      232-1      No enable    RW   UINT32
                                                                                                   effective
                                                                                                   Running
                                                                                                    setting
6091     01   Motor resolution       1～2147483647                             -          1         Effective   RW   UINT32
                                                                                                  immediate
                                                                                                       ly
                                                                                                   Running
                                                                                                    setting
6091     02   Axis resolution        1～2147483647                             -          1         Effective   RW   UINT32
                                                                                                  immediate
                                                                                                       ly
                                                                                                   Running
6098     00   Zero return method     1～35；Reference zero return mode          -          0          setting    RW    INT8
                                     Appendix 2                                                   No enable
                                                                                                   effective
                                                                                                   Running
6099     01   Return to Zero         0～2147483647                         command      131072       setting    RW   UINT32
              Expressway                                                   unit /S                No enable
                                                                                                   effective
                                                                                                   Running
6099     02   Return to zero low     10～2147483647)                       command      13107        setting    RW   INT32
              speed                                                        unit /S                No enable
                                                                                                   effective
                                                                                                   Running
              Return to zero                                              command                   setting         DUINT3
609A     00   acceleration           0～2147483647                          unit /S2    100000     No enable    RW     2
                                                                                                   effective
                                                                                                   Running
                                                                          command                   setting
60B0     00   position offset        -2147483648～2147483647                 unit         0        No enable    RW   INT32
                                                                                                   effective
                                                                                                   Running
                                                                          command                   setting
60B1     00   speed offset           -2147483648～2147483647                unit /S       0        No enable    RW   INT32
                                                                                                   effective
                                                                                                   Running
60B2     00   Torque bias            -5000～5000                             0.1%         0          setting    RW   INT16
                                                                                                  No enable
                                                                                                   effective
  -44-


---

<!-- PDF page 49 -->

EtherCAT 总线伺服驱动器用户手册
                                                                                           Running
60B8   00   probe mode            0～65535                                   -        0      setting    RW   UINT16
                                                                                           No enable
                                                                                           effective
60B9   00   Probe status          0～65535                                   -        0         -       RO   UINT16

            Probe 1 rising edge                                          command
60BA   00   position value        -2147483648～2147483647                   unit      0         -       RO   INT32


60BB   00   Probe 1 falling       -2147483648～2147483647                 command     0         -       RO   INT32
            edge position value                                            unit

60BC   00   Probe 2 rising edge   -2147483648～2147483647                 command     0         -       RO   INT32
            position value                                                 unit

60BD   00   Probe 2 falling       -2147483648～2147483647                 command     0         -       RO   INT32
            edge position value                                            unit
                                                                                           Running
60E0   00   Forward torque        0～5000                                  0.1%      5000    setting    RW   UINT16
            limit                                                                          No enable
                                                                                           effective
                                                                                           Running
            Reverse torque                                                                  setting
60E1   00   limit                 0～5000                                  0.1%      5000   No enable   RW   UINT16
                                                                                           effective
                                  bit0~bit7: The lower 8 bits are used
                                  to display the supported zero return
       01   Supported zero        method. Bit8: Whether to support
60E3   ～    return method         relative position zero return             -        -         -       RO   UINT16
       1F   1~Supported zero      Bit9: Whether to support absolute
            return method 31      position zero return
                                  0: Absolute position return to zero.
                                  After the origin return is
                                  completed, position feedback 6064
                                  is set to origin offset 607Ch;                           Running
60E6   00   Actual position       1: Relative position return to zero.      -        0      setting    RW   UINT8
            calculation method    After the origin return is                               No enable
                                  completed, the position feedback                         effective
                                  6064 will superimpose the position
                                  offset 607Ch on the original basis.
60F4   00   position deviation    Display position deviation             command     -         -       RO   DINT32
                                                                           unit
                                  Position command 60FC (encoder unit)   Encoder
60FC   00   position command      = position command 6062 (command         unit      -         -       RO   DINT32
                                  unit) ×electronic gear ratio (6091)

                                     Bit            explain
                                     0       Reverse overtravel
                                             switch
                                     1       Forward overtravel
                                             switch
                                     2       Origin switch
                                     16      ZSignal
60FD   00   DI state                 17      probe1                         -        0         -       RO   DINT32
                                     18      probe2
                                     20      DI0
                                     21      DI1
                                     22      DI2
                                     23      DI3
                                     24      DI4
                                                                                           Running
                                  Bit0 of 60FE-01h: brake output;                           setting
60FE   01   physical output       when 200D-12h=4, the DO output            -        0                 RW   UINT32
                                                                                           No enable
                                  is controlled by the bits of                             effective
                                  60FE-01h and 60FE-02h:
                                            60FE-01h 60FE-02h
                                    DO0 Bit16             Bit16                            Running
60FE   02   Physical output                                                 -        0      setting    RW   UINT32
            enable                  DO1 Bit17             Bit17                            No enable
                                    DO2 Bit18             Bit18                            effective

                                                                                           Running
                                                                         command            setting
60FF   00   target speed          -2147483648～2147483647                  unit /S    0     No enable   RW   INT32
                                                                                           effective

                                                                                                               -45-


---

<!-- PDF page 50 -->

EtherCAT 总线伺服驱动器用户手册

                                       Chapter IX Troubleshooting

9.1 Fault and Warning Code List

9.1.1 Fault code table (to reset the fault, you need to cancel the enable first)
           Error
Display     code       Fault name      Reset                                   Fault and handling method
          (603Fh)
                                           1. The function code parameter value of P02 and following sets exceeds the upper and lower
                 Parameters of P02         limits, and the parameters are re-initialized;
 Er.101   0x6320 and above sets are     No 2. Power off during the process of writing parameters, rewrite the parameters after power on;
                 abnormal                  3. Reset the motor model and drive model, and initialize the parameters;
                                           4. The drive EEPROM is abnormal, replace the drive.
                 Programmable
 Er.102   0x7500 logic configuration    No MCU related hardware is damaged, replace the drive.
                 failure
                 Programmable
 Er.104   0x7500 logic interrupt        No MCU related hardware is damaged, replace the drive.
                 failure
                                           1. When EEPROM reads/writes function codes, the total number of function codes is
                 Abnormal internal         abnormal, initialize the parameters;
 Er.105   0x6320 program                No 2. The range of the set value of the function code is abnormal, initialize the parameters;
                                           3. Initialize and power on again. If the alarm still occurs, replace the drive.
                 Parameter storage         1. The parameter value can’t be written to the EEPROM, initialize the parameter;
 Er.108   0x5530 failure                No 2. Initialize and power on again. If the alarm still occurs, replace the drive.

 Er.111   0x6320 Internal failure       No Initialize and power on again. If the alarm still occurs, replace the drive.
                 Product matching          The motor model and drive model match incorrectly, please contact the after-sales personnel
 Er.120   0x7122 failure                No to check the motor model.
                 Servo ON
 Er.121   0x5441 command invalid        Yes DI port parameter configuration fault, recheck DI function and VDI function configuration
                 fault
                 Absolute position
 Er.122   0x7122 mode product           No The absolute value motor model does not match, or the motor model is set incorrectly, please
                 matching failure          contact the after-sales personnel to check the motor model.
                 Duplicate
 Er.130   0x6320 assignment of DI       Yes DI port parameter configuration failure, recheck the DI function and VDI function
                 function                   configuration or initialize parameters.

 Er.131   0x6320 DO function            Yes DO function number exceeds DO function number, recheck DO function configuration or
                 allocation overrun         initialize parameters.
                                           When the drive reads the parameters in the encoder ROM area, it finds that the parameters are
                 The data in motor         not saved, or the parameters are inconsistent with the agreed values
                 ROM is incorrectly        1. Check the motor model and drive model;
 Er.136   0x7305 verified or the        No 2. Check whether the motor encoder cable is correct, and whether the connector is connected
                 parameters are not        reliably;
                 saved                     3. Check if the encoder line is disturbed, and re-arrange the wires.

                                           Overcurrent detected by hardware;
                                           1. Check whether the motor power lines U V W are correctly connected, and whether there is
                                           a reverse connection or phase loss;
                                           2. There is a short circuit in the U V W lines, or there is leakage between the motor coil and
                                           the casing, replace the motor wire or test the motor;
                                           3. The encoder line is in poor contact, check or replace the encoder cable;
 Er.201   0x2312 Overcurrent 2          No 4. The load is too heavy, first test whether the motor is normal with no load;
                                           5. The acceleration and deceleration are too fast, increase the acceleration and deceleration
                                           time of the program;
                                           6. If the gain parameter is adjusted, check whether the gain is set too large, and test after
                                           reducing the gain;
                                           7. The braking resistor is too small or short-circuited, test with internal braking resistor first;
                                           8. The drive is damaged, replace the drive;

                 D/Q axis current           Abnormal current feedback causes the internal register of the drive to overflow, replace the
 Er.207   0x0FFF overflow fault         Yes drive;




   -46-


---

<!-- PDF page 51 -->

EtherCAT 总线伺服驱动器用户手册
                                             1. MCU communication timeout, replace the drive
                                             2. Encoder communication times out, check whether the encoder line is connected well, or
                                             replace the encoder and reconnect;
                                             3. Motor encoder is faulty, replace the motor for test;
Er.208   0x0FFF System sampling        No    4. Current sampling times out, check whether there is interference from large equipment on
                operation timeout            site, increase the isolation transformer, and re-arrange the wires;
                                             5. High-precision AD conversion times out, check the analog input wiring to see if there is
                                             interference, and connect with shielded wire;
                                             6. The drive is damaged, replace the drive;
                                             During the power-on self-test of the drive, the motor phase current or bus voltage is detected
                                             abnormal.
Er.210   0x2330 Output short circuit   No    1. The power lines U V W are short-circuited to the ground, check the motor lines;
                to ground                    2. The motor coil is short-circuited to the casing, replace the motor;
                                             3. Drive failure, replace the drive.
                                             The drive performs angle identification, and it is identified that the phase sequence of the
                                             UVW of the drive and the UVW of the motor do not match.
Er.220   0x0FFF Phase sequence         No    1. The electrical angle of the motor encoder does not match, reset the motor parameters, and
                error                        self-learn;
                                             2. The U V W phase sequence is reversed, check the motor power lines;
                                             In torque control mode, the direction of the torque command is opposite to the direction of the
                                             speed feedback or in the position or speed control mode, the direction of the speed feedback is
                                             opposite to the direction of the speed command;
Er.234   0x0FFF Overspeed              No    1. The U V W phase sequence is reversed, check the motor power lines;
                                             2. Initial phase detection error of the motor rotor is caused by the interference signal,
                                             re-power on, and check the wiring;
                                             3. The encoder model is wrong or the wiring is wrong, replace the motor or encoder line;
                                             4. Drive failure, replace the drive;
                                             DC bus voltage exceeds fault value 420V
                                             1. Measure the power supply voltage. If the grid voltage is too high or unstable, a voltage
                                             stabilizer needs to be added;
                                             2. The braking resistor fails, measure the resistance between B1 and B3 of the drive in the
                                             state of complete power failure. If it is infinite, the internal braking resistor is damaged and
                Main circuit                 the drive needs to be replaced;
Er.400   0x3210 overvoltage            Yes   3. The resistance of the braking resistor is too large, replace it with a braking resistor of 40
                                             ohms or 50 ohms, please contact the after-sales personnel;
                                             4. The grid voltage is too high, and the motor accelerates and decelerates too fast, increase the
                                             acceleration and deceleration time;
                                             5. Monitor P0B-26 to check whether the bus voltage is consistent with the grid voltage. If the
                                             difference is too large, the drive may be damaged and needs to be replaced. 220V AC
                                             corresponds to the bus voltage of 310V.
                                             DC bus voltage is lower than the fault value 200V
                                             1. The main circuit power supply is unstable or power off, re-check the wiring, or add a
Er.410   0x3220 Main circuit           Yes   voltage stabilizer;
                undervoltage                 2. Monitor P0B-26 to check whether the bus voltage is consistent with the grid voltage. If the
                                             difference is too large, the drive may be damaged and needs to be replaced. 220V AC
                                             corresponds to the bus voltage of 310V.
Er.420   0x3130 Main circuit power     Yes Servo drive failure, replace the drive.
                phase loss
Er.430   0x3120 Control power          Yes Servo drive failure, replace the drive.
                undervoltage
                                             The actual speed of the servo motor exceeds the overspeed fault threshold
                                             1. The phase sequence of motor cable U V W is wrong, check the motor wiring;
                                             2. The motor parameters are incorrect, reset the motor parameters and self-learn;
Er.500   0x8400 Overspeed alarm        Yes   3. The input command exceeds the overspeed fault threshold;
                                             4. The motor speed is overregulated, the gain parameter setting is unreasonable, initialize the
                                             drive parameters and test;
                                             5. Drive failure, replace the drive.
                Pulse output                 The output pulse frequency exceeds the upper limit of the frequency allowed by the hardware;
Er.510   0x0FFF overspeed              Yes   reduce P05-17 (number of pulses divided by the encoder frequency), so that the output pulse
                                             frequency is less than the upper limit of the allowable frequency.
                Angle identification         Motor self-learning failed, check whether the encoder line is normal and the encoder type is
Er.602   0x0FFF failed               Yes     correct.
                                             1. The motor model or drive model is set incorrectly, please contact the after-sales personnel
                                             to check the parameters;
Er.610   0x3230 Drive overload         Yes   2. Monitor the drive load rate PB-02 to see if the overload causes an alarm;
                                             3. The motor is stalled, first eliminate the motor stall and then test, or remove the motor for
                                             no-load test;
                                             4. The gain parameter setting is too large, test after initializing the parameters;
                                             5. Motor acceleration and deceleration is too fast, increase the acceleration and deceleration
Er.620   0x3230 Motor overload         Yes   time;
                                             6. The phase sequence of motor cable U V W is wrong, check the motor wiring;
                                             7. The drive is damaged, replace the drive.




                                                                                                                                          -47-


---

<!-- PDF page 52 -->

EtherCAT 总线伺服驱动器用户手册
                                           The actual speed of the motor is lower than 10rpm, but the torque command reaches the limit
                                           value, and the duration reaches the set value of P0A-32
                                           1. The UVW output of the drive is out of phase, disconnected, and wrongly connected in
Er.630   0x7121 Motor stall            Yes phase sequence;
                                           2. The motor parameters are incorrect, reset the motor parameters and self-learn;
                                           3. The motor is stalled, first eliminate the motor stall and then test, or remove the motor for
                                           no-load test;
                Heat sink                  The temperature of the power module of the drive is higher than the over-temperature
Er.650   0x4210 overheating            Yes protection point, the servo drive is faulty, replace the drive.

                Encoder battery            The battery voltage of the absolute value encoder is lower than 3.0V
Er.731   0x7305 failure                Yes 1. The encoder line is disconnected, set P0D-20=2, and then set P0D-01=1 to clear the fault;
                                           2. The battery is dead, replace the battery.

                                           Initialize the drive parameters, reset the motor parameters and drive parameters, set the
Er.733   0x7305 Encoder multi-turn     Yes encoder type,
                count error                Then set P0D-20=2 and P0D-01=1 to clear the fault and power on again. If the alarm still
                                           occurs, replace the motor and test.

                                           Initialize the drive parameters, reset the motor parameters and drive parameters, set the
Er.735   0x7305 Encoder multi-turn     Yes encoder type,
                count overflow             Then set P0D-20=2 and P0D-01=1 to clear the fault and power on again. If the alarm still
                                           occurs, replace the motor and test.
                                          Encoder Z signal is interfered, causing the electrical angle corresponding to the Z signal to
                                          change too much
                Encoder                   1. The encoder wiring is wrong or the connector is loose, check or replace the encoder line
Er.740   0x7305 interference           No and test it;
                                          2. Encoder Z signal is disturbed, re-wire and ensure a good grounding;
                                          3. The encoder is faulty, replace the motor;

                Encoder data              The internal parameters of the encoder are abnormal
Er.A33   0x7305 abnormal               No 1. The serial encoder line is disconnected or loose, check or replace the encoder line and test;
                                          2. The encoder is faulty, replace the motor;
                Encoder loopback          1. The driver and motor types do not match, reset the motor model;
Er.A34   0x7305 verification           No 2. The encoder line is broken, check the encoder line.
                abnormal
                                          Encoder Z signal is lost or the AB signal edge transitions at the same time
Er.A35   0x7305 Z signal loss          No 1. The serial encoder line is disconnected or loose, check or replace the encoder line and test;
                                          2. The encoder is faulty, replace the motor;
                                           In position control mode, the position deviation is greater than the P0A-10 setting value
                                           1. The driver U V W output phase is missing or the phase sequence is wrongly connected,
                                           check the motor wire;
                                           2. If the motor is stalled, first rule out the motor stall condition before testing, or remove the
                Excessive position         motor and test without load;
Er.B00   0x8611 deviation              Yes 3. The gain of the servo driver is low, so test after initializing the parameters;
                                           4. The position command increment is too large;
                                           5. Whether the position deviation fault value 6065h is set too small;
                                           6. The torque limit value is set too small, test after initializing the parameters;
                                           7. Servo driver/motor failure, replace the driver or motor.
                                             1. Is the position deviation fault value 6065h set too small?
                                             2. Before mode switching or when servo is enabled, the target position (607A target
                  Position command           position) is not aligned with the current position;
Er.B01 0x0FFF     too large              yes 3. Synchronization cycle phase crossover leads to excessive position command
                                             accumulation;
                                             4. Motor speed limit error;
                  Electronic        gear
Er.B03   0x6320   ratio          setting yes 1. The gear ratio 6091-01h/6091-02h exceeds the limit value;
                  exceeds limit              2. Parameter change order problem;
                  Software position
Er.D09   0x6320   upper and lower yes The software position upper and lower limit settings are wrong, check 0x607D-01h and
                  limit setting error        0x607D-02h
                  Origin          offset     The origin offset is outside the upper and lower limits of the software position, check
Er.D10   0x6320   setting error          yes 0x607D-01h, 0x607D-02h, 0x607Ch
                  Abnormal network
Er.E07   0x0FFF   status switching       yes Check whether the network port is normal and whether the communication line is normal;
                                             1. The slave station receives abnormally, check whether the network port is normal and
Er.E08   0x0FFF   Sync lost              yes whether the communication line is normal;
                                             2. The master station sends abnormally, and the upper computer synchronization clock error
                                             is too large. You can try increasing 200E-21h;
Er.E11   0x0FFF   XML configuration yes 1. The device configuration file is not burned;
                  file not burned            2. Drive failure;
Er.E12   0x0E12   Network                yes 1. The device configuration file is not burned;
                  initialization failed      2. Drive failure;
Er.E13   0x0E13   Synchronization        yes Check whether the synchronization period is 125us or an integer multiple of 250us
                  cycle setting error

  -48-


---

<!-- PDF page 53 -->

EtherCAT 总线伺服驱动器用户手册
                Synchronization        1.XML files do not match;
  Er.E15 0x0E15 cycle error is too yes 2. The controller synchronization cycle error is large;
                large




9.1.2 Warning code table (warnings can be reset directly without canceling the enable)
           Error
Display     code        Fault name        Reset                                    Fault and handling method
          (603Fh)
                    Frequency division            The number of encoder frequency-divided pulses does not meet the range, reset the
 Er.110   0x6320       pulse output        yes    encoder frequency-divided pulse number (2005-12h);
                      setting failure
                                                  1. Origin switch failure;
 Er.601   0x0FFF    Failed to return to    yes    2. The time limit for searching the origin is too short;
                          origin                  3. The speed of high-speed search for origin switch signal is too small;
                                                  4. The switch setting is unreasonable;
 Er.730   0x7305     Encoder battery       yes    The encoder battery voltage of the absolute encoder is lower than 3.0V. Replace the
                        warning                   battery with a new one with matching voltage while the power is on.

 Er.900   0x5442      DI emergency         yes    The corresponding DI terminal of DI function 34 (FunIN.34: Brake, Emergency) is
                          brake                   triggered (including hardware DI and virtual DI), check the DI wiring.
                     Motor overload               The load rate is too high, causing a warning. Check whether the load is too heavy or
 Er.909   0x3230       warning             yes    blocked.
                                                  Warning of excessive braking resistor current,
                                                  1. If the bus voltage is too high, causing energy to be discharged too quickly, a warning
                                                  will appear. Add a voltage regulator to reduce the voltage;
                                                  2. Whether the motor decelerates too fast, increase the deceleration time;
 Er.920   0x3210     Braking resistor      yes    3. The internal braking resistor has insufficient power. Replace the external braking
                        overload                  resistor. It is recommended that the resistance value should not be lower than 40 ohms;
                                                  4. When using an external resistor, check the parameter values of P02-25 ~ P2-27, and set
                                                  the value of P2-27 to be consistent with the resistance value of the selected resistor;
                                                  4. The driver braking circuit is damaged, replace the driver;
                     External braking             P02-27 (resistance value of external braking resistor) is smaller than P02-21 (minimum
 Er.922   0x6320      resistor is too      yes    value of external braking resistor allowed by the driver)
                          small
                    Motor power line              The actual phase current of the motor is less than 10% of the rated current, and the actual
 Er.939   0x3331       is broken           yes    speed is small, but the internal torque command is large. Check the motor power cable
                                                  wiring, rewire it, and replace the cable if necessary.
                    Parameter changes             When the function code attribute "validity time" of the servo drive is "power on again",
 Er.941   0x6320        need to be         yes    after the function code parameter value is changed, the drive reminds the user that he
                    powered on again              needs to power on again.
                      to take effect.
                                                  If the number of function codes modified at the same time exceeds 200, check the
 Er.942   0x7600      Parameters are       yes    operating mode. For parameters that do not need to be stored in EEPROM, set P0C-13 to
                     stored frequently            0 before the host computer writes the operation.
 Er.950   0x5443 Forward overtravel        yes    The corresponding DI terminal of DI function 14 (FunIN.14: P-OT, forward overtravel
                      warning                     switch) is triggered.
                 Reverse overtravel               The corresponding DI terminal of DI function 15 (FunIN.15: N-OT, reverse overtravel
 Er.952   0x5444      warning              yes    switch) is triggered.
                  Encoder internal
 Er.980   0x7305        fault              yes    If a fault still occurs after turning on the power several times, the encoder is faulty.
                  Input phase loss
 Er.990   0x3130      warning              yes    Driver power supply circuit failure;
                  Zero return mode                When using the zero return mode, 6098h inputs non-existent zero return modes such as
 Er.998   0x0FFF    setting error          yes    15/16/31/32.
                                                  Motor self-learning failed,
Er.A40 0x0FFF         Internal failure     yes    1. Check the motor encoder line error;
                                                  2. The encoder model is incorrect. Reset the motor model and encoder type;
                                                  3. The motor encoder is faulty, replace the motor.




                                                                                                                                             -49-


---

<!-- PDF page 54 -->

EtherCAT 总线伺服驱动器用户手册

                                 Appendix 1 Shutdown method
Object                                                                                             D    Setting     Att   t
dictio    Sub    Na                                      Scope                                     ef    takes      rib   y
 nary    index   me                                                                                au    effect     ute   p
                                                                                                   lt                s    e
                       PPmodel：
                         set value                           shutdown mode
                             0       Free stop and maintain free running status
                             1       Stop at 6084h ramp and maintain free running status
                             2       Stop at 6085h ramp and maintain free running status
                                     Stop with 2007-10h emergency stop torque and maintain free
                             3       running state
                             5       Stop at 6084h ramp and maintain position locked state
                             6       Stop at 6085h ramp and maintain position locked state
                             7       Stop with 2007-10h emergency stop torque and maintain
                                     position locked state
                       CSPmodel：
                         set value                         shutdown mode
                             0     Free stop and maintain free running status
                 Qu          1
                 ick               Stop with 2007-10h emergency stop torque and maintain free
                             2
                                   running state
                 Sh          3
                 ut          5
                                   Stop with 2007-10h emergency stop torque and maintain
                             6     position locked state                                                Running           I
                 Do          7                                                                           setting          N
605A      00     wn                                                                                2       No       R     T
                       CSV/PV/HM model：                                                                             W
                         set value                         shutdown mode                                 enable           1
                 Mo          0     Free stop and maintain free running status                           effective         6
                 de                Stop at 6084h (HM: 609Ah) ramp and maintain free running
                             1
                                   status
                 sel         2     Stop at 6085h ramp and maintain free running status
                 ect         3     Emergency stop torque to stop and maintain free running state
                 ion
                                   Stop at 6084h (HM: 609Ah) slope and maintain position locked
                             5     state
                             6     Stop at 6085h ramp and maintain position locked state
                                   Stop with 2007-10h emergency stop torque and maintain
                             7     position locked state
                       CST/PT model:
                         set value                         shutdown mode
                             0     Free stop and maintain free running status
                             1
                                   Stop at 6087h ramp and maintain free running status
                             2
                             3     Free stop and maintain free running status
                             5
                                   Stop at 6087h ramp and maintain position locked state
                             6
                             7     Coast to stop and maintain position lock state

                       PP mode：
                         set value                          shutdown mode
                 Te          1       Stop at 6084h ramp and maintain free running status
                 m           2       Stop at 6085h ramp and maintain free running status
                 po                  Stop with 2007-10h emergency stop torque and maintain free
                 rar         3
                                     running state
                 y
                       CSP mode：
                 Sh      set value                         shutdown mode
                 ut          1
                             2     Stop with 2007-10h emergency stop torque and maintain free           Running           I
                 do                running state                                                         setting          N
                 w           3                                                                                      R
605D      00                                                                                       1       No             T
                 n     CSV/PV/HM mode：                                                                   enable     W     1
                         set value                         shutdown mode                                effective         6
                 M                 Stop  at 6084h (HM:   609Ah)   ramp and maintain free running
                 et          1
                                   status
                 ho          2     Stop at 6085h ramp and maintain free running status
                 d           3     Emergency stop torque to stop and maintain free running state
                 sel   CST/PT mode:
                 ect     set value                         shutdown mode
                 ion         1
                                   Stop at 6087h ramp and maintain free running status
                             2
                             3     Free stop and maintain free running status


-50-


---

<!-- PDF page 55 -->

EtherCAT 总线伺服驱动器用户手册

                                 Appendix 2 Servo home mode
6098=1: Reference negative limit and return-to-origin mode of Z-phase signal
Situation 1: The negative limit is invalid when starting to return to zero, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the negative limit, the motor
         decelerates and runs in the positive direction at low speed. When encountering the negative limit, it
         decreases. The position of the first Z pulse after the edge is the origin position.
Situation 2: The negative limit is valid when starting to return to zero, and the axis begins to return to zero in the
         positive direction at low speed. When encountering the negative limit falling edge, the position of the first Z
         pulse is the origin position.
                                                 负限位开关
                                                  Negative limit switch




                                                                     ①

                                       ②
                              Z信号
                            Z signal

                     Negative limit
                            负限位



6098=2: Reference positive limit and Z-phase signal return-to-origin mode
Situation 1: The positive limit is invalid when starting to return to zero, and the axis begins to return to zero at high
         speed in the positive direction. When encountering the rising edge of the positive limit, the motor
         decelerates and runs in the negative direction at low speed. When encountering the positive limit The
         position of the first Z pulse after the falling edge is the origin position.
Scenario 2: The positive limit is valid when starting to return to zero, and the axis begins to return to zero in the
         negative direction at a low speed. When encountering the falling edge of the positive limit, the position of the
         first Negative
               Z pulse islimit
                           the origin position.
                                                   Positive 正限位开关
                                                            limit switch




                                                     ①

                                                                                   ②
                            Z signal
                              Z信号

                            正限位
                        Positive limit

6098=3: Reference origin switch and forward return-to-origin mode of Z-phase signal
Situation 1: When starting the zero return, the origin signal is invalid, and the axis begins to return to zero at high
         speed in the positive direction. When encountering the rising edge of the origin, the motor decelerates and
         runs in the negative direction at low speed. When encountering the falling edge of the origin, the motor The
         position of one Z pulse is the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis begins to return to zero in the
         negative direction at a low speed. When it encounters the falling edge of the origin, the position of the first Z
         pulse is the origin position.
                                                                                                                      -51-


---

<!-- PDF page 56 -->

EtherCAT 总线伺服驱动器用户手册

                                                              原点开关
                                                           Origin switch



                                                    ①

                                                                               ②
                             ZZ信号
                               signal


                               原点
                              origin

6098=4: Reference origin switch and forward return-to-origin mode of Z-phase signal
Situation 1: When starting the zero return, the origin signal is invalid, and the axis begins to return to zero in the
         positive direction at high speed. When encountering the rising edge of the origin, the motor decelerates and
         runs at low speed. The position when encountering the first Z pulse is the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis starts to return to zero at a low
         speed in the negative direction. When it encounters the falling edge of the origin, it decelerates and runs in
         the positive direction at a low speed. When it encounters the first rising edge of the origin, The position of
         the Z pulse is the origin position.
                                                        Origin原点开关
                                                              switch




                                                    ①
                                                                               ②


                              Z信号
                            Z signal


                               原点
                               Origin
6098=5: Negative return-to-origin mode of reference origin switch and Z-phase signal
Situation 1: When the zero return is started, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the origin, the motor decelerates and
         runs in the positive direction at low speed. When encountering the falling edge of the origin, the motor The
         position of one Z pulse is the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis begins to return to zero at a low
         speed in the positive direction. The position of the first Z pulse after encountering the falling edge of the
         origin is the origin position.
                                                   Origin switch
                                                   原点开关



                                                                      ①

                                           ②
                              Z信号
                           Z signal

                               原点
                               Origin
    -52-


---

<!-- PDF page 57 -->

EtherCAT 总线伺服驱动器用户手册
6098=6: Negative return-to-origin mode of reference origin switch and Z-phase signal
Situation 1: When starting the zero return, the origin signal is invalid, and the axis begins to return to zero in the
         negative direction at high speed. When encountering the rising edge of the origin, the motor decelerates
         and runs at low speed. The position when encountering the first Z pulse is the origin position.
Situation 2: When starting the zero return, the origin signal is valid. The axis starts to return to zero at a low speed in
         the positive direction. When it encounters the falling edge of the origin, it decelerates and runs in the
         negative direction at a low speed. When it encounters the first Z after the origin signal The position of the
         pulse is the origin position.
                                                    原点开关
                                                    Origin switch




                                                                       ①
                                            ②

                              Z信号
                              Z signal

                               原点
                               Origin

6098=7: Reference origin switch, positive limit and Z-phase signal return-to-origin mode 1
Situation 1: When starting the zero return, the origin signal is invalid, and the axis begins to return to zero at high
         speed in the positive direction. When encountering the rising edge of the origin, the motor decelerates and
         runs in the negative direction at low speed. When encountering the falling edge of the origin, the motor The
         position of one Z pulse is the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis begins to return to zero in the
         negative direction at a low speed. When it encounters the falling edge of the origin, the position of the first Z
         pulse is the origin position.
Situation 3: When the zero return is started, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the positive direction. When encountering the rising edge of the positive limit, the motor
         decelerates and runs in the negative direction at high speed. When encountering the rising edge of the
         origin, , the motor starts to decelerate and run at low speed. When encountering the falling edge of the
         origin, the position of the first Z pulse is the origin position.
                                        Origin
                                           原点开关switch                 Positive正限位
                                                                               limit




                                           ①

                                                          ②                  ③

                             Z Z信号
                               signal

                               原点
                             Origin

                     Positive正限位
                              limit
6098=8: Reference origin switch, positive limit and return-to-origin mode 2 of Z-phase signal
Situation 1: When the zero return is started, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the positive direction. When encountering the rising edge of the origin, the motor decelerates and
         runs in the negative direction at low speed. When it encounters the falling edge of the origin, it decelerates
         again. Reverse direction, and then run in the forward direction at low speed. When encountering the rising
         edge of the origin, the position to the first Z pulse is the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis starts to return to zero at a low
                                                                                                                       -53-


---

<!-- PDF page 58 -->

EtherCAT 总线伺服驱动器用户手册
         speed in the negative direction. When it encounters the falling edge of the origin, it decelerates and runs in
         the positive direction at a low speed. When it encounters the first rising edge of the origin, The position of
         the Z pulse is the origin position.
Situation 3: When the zero return is started, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the positive direction. When encountering the rising edge of the positive limit, the motor
         decelerates and runs in the negative direction at high speed. When encountering the rising edge of the
         origin, , the motor starts to run at low speed. When it encounters the falling edge of the origin, the motor
         decelerates and reverses and runs toward the square at low speed. When it encounters the rising edge of
         the origin, the position of the first Z pulse is the origin position.

                                         原点开关
                                       Origin switch                          正限位
                                                                       Positive limit



                                           ①


                                                              ②
                                                                             ③



                               Z信号
                            Z signal

                               原点
                             Origin


                      Positive正限位
                               limit


6098=9: Reference origin switch, positive limit and return-to-origin mode 3 of Z-phase signal
Situation 1: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero in the
         forward direction at high speed. When encountering the rising edge of the origin, the motor decelerates and
         runs forward at low speed. After encountering the falling edge of the origin, the motor reverses and runs at
         low speed. Running in the negative direction, the position of the first Z pulse after encountering the rising
         edge of the origin is the origin position.
Situation 2: When the zero return starts, the origin signal is valid, and the axis starts to return to zero at a low speed
         in the positive direction. When encountering the falling edge of the origin, the motor decelerates and runs in
         the negative direction at low speed. After encountering the first rising edge of the origin, The position of the
         first Z pulse is the origin position.
Situation 3: When starting the zero return, the origin signal is invalid. The axis starts to return to zero at high speed in
         the positive direction. When encountering the rising edge of the positive limit, the motor decelerates and
         runs in the negative direction at high speed. After encountering the rising edge of the origin, the motor The
         deceleration reverse direction moves in the forward direction at a low speed. After encountering the falling
         edge of the origin, it reverses and then moves in the negative direction at a low speed. The position of the
         first Z pulse after encountering the rising edge of the origin is the origin position.




    -54-


---

<!-- PDF page 59 -->

EtherCAT 总线伺服驱动器用户手册

                                           原点开关
                                        Origin switch                  Positive正限位
                                                                                limit



                                            ①

                                                    ②
                                                                             ③




                               Z信号
                             Z signal

                               原点
                              Origin

                      Positive正限位
                               limit

6098=10: Reference origin switch, positive limit and Z-phase signal return-to-origin mode 4
Situation 1: The origin signal is invalid when starting the zero return. The axis starts to return to zero in the forward
         direction at high speed. When encountering the rising edge of the origin, the motor decelerates and runs at
         low speed. When encountering the falling edge of the origin, the first Z pulse The position is the origin
         position.
Situation 2: When the origin signal is valid when starting the zero return, the axis starts to return to zero at a low
         speed in the forward direction. The position of the first Z pulse when encountering the falling edge of the
         origin is the origin position.
Situation 3: The origin signal is invalid when starting to return to zero. The axis starts to return to zero in the positive
         direction at high speed. When encountering the rising edge of the positive limit, the motor decelerates and
         runs in the negative direction at high speed. When encountering the rising edge of the origin opening,
         When , the motor decelerates and runs in the forward direction at low speed. When it encounters the falling
         edge of the origin, the position of the first Z pulse is the origin position.

                                          原点开关
                                        Origin switch                         正限位
                                                                       Positive limit



                                            ①

                                                    ②
                                                                             ③



                               Z信号
                            Z signal

                               原点
                              Origin

                     Positive正限位
                              limit
6098=11: Reference origin switch, negative limit and Z-phase signal return-to-origin mode 1
Situation 1: When starting the zero return, the origin signal is invalid. The axis starts to return to zero at high speed in
         the negative direction. When encountering the rising edge of the origin, the motor decelerates and runs in
         the forward direction at low speed. When encountering the falling edge of the origin, the motor The position
         of one Z pulse is the origin position.
                                                                                                                        -55-


---

<!-- PDF page 60 -->

EtherCAT 总线伺服驱动器用户手册
Situation 2: When the origin signal is valid when starting the zero return, the axis starts to return to zero at a low
         speed in the forward direction. The position of the first Z pulse after encountering the falling edge of the
         origin is the origin position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When it encounters the rising edge of the negative limit, the motor
         decelerates and runs in the positive direction at high speed. When it encounters the rising edge of the
         origin, , the motor decelerates and runs at low speed. When encountering the falling edge of the origin, the
         position of the first Z pulse is the origin position.

                                          负限位
                                          Negative limit                    原点开关
                                                                            Origin switch




                                                                                    ①

                                                  ③                 ②

                              Z信号
                            Z signal

                               原点
                              Origin

                     Negative limit
                            负限位
6098=12: Reference origin switch, negative limit and return-to-origin mode 2 of Z-phase signal
Situation 1: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the origin, the motor decelerates and
         runs in the forward direction at low speed. When encountering the falling edge of the origin, the motor The
         deceleration reverse direction runs in the negative direction at low speed. When encountering the rising
         edge of the origin, the position of the first Z pulse is the origin position.
Situation 2: When starting the zero return, the origin signal is valid, and the axis starts to return to zero at a low speed
         in the positive direction. When encountering the falling edge of the origin, the motor decelerates and runs in
         the negative direction at a low speed. When it encounters the rising edge of the origin, the The position of
         one Z pulse is the origin position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the negative limit, the motor
         decelerates and moves in the positive direction at high speed. When encountering the rising edge of the
         origin, , the motor decelerates and runs at low speed. When it encounters the falling edge of the origin, the
         motor decelerates and runs in the negative direction at low speed. When it encounters the rising edge of the
         origin, the position of the first Z pulse is the origin position.

                                           负限位
                                            Negative limit                 原点开关
                                                                            Origin switch



                                                                                   ①


                                                                ②
                                                 ③



                               Z信号
                            Z signal

                                原点
                              Origin
                     Negative limit
                            负限位

    -56-


---

<!-- PDF page 61 -->

EtherCAT 总线伺服驱动器用户手册
6098=13: Reference origin switch, negative limit and return-to-origin mode 3 of Z-phase signal
Situation 1: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the origin, the motor decelerates and
         runs at low speed. When encountering the falling edge of the origin, the motor decelerates and reverses to
         low speed. Running in the forward direction, the position of the first Z pulse after encountering the rising
         edge of the origin is the origin position.
Situation 2: When the zero return starts, the origin signal is valid, and the axis starts to return to zero at a low speed
         in the negative direction. When encountering the falling edge of the origin, the motor decelerates and runs
         in the forward direction at low speed. When it encounters the rising edge of the origin, the The position of
         one Z pulse is the origin position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When it encounters the rising edge of the negative limit, the motor
         decelerates and runs in the positive direction at high speed. When it encounters the rising edge of the
         origin, , the motor decelerates and runs in the negative direction at low speed. When it encounters the
         falling edge of the origin, the motor decelerates and runs in the forward direction at low speed. When it
         encounters the rising edge of the origin, the position of the first Z pulse is the origin position.
                                             负限位
                                             Negative limit                原点开关
                                                                            Origin switch


                                                                                  ①

                                                                          ②
                                                  ③




                                  Z信号
                              Z signal

                                  原点
                               Origin

                       Negative负限位
                               limit
6098=14: Reference origin switch, negative limit and return-to-origin mode 4 of Z-phase signal
Situation 1: The origin signal is invalid when starting to return to zero. The axis starts to return to zero in the negative
     direction at high speed. When encountering the rising edge of the origin, the motor decelerates and runs at low
     speed. When encountering the falling edge of the origin, the first Z pulse The position is the origin position.
Situation 2: When the origin signal is valid when the zero return is started, the axis starts to return to zero at a low
     speed in the negative direction. The position of the first Z pulse after encountering the falling edge of the origin is
     the origin position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
     speed in the negative direction. When it encounters the rising edge of the negative limit, the motor decelerates
     and runs in the positive direction at high speed. When it encounters the rising edge of the origin, , the motor
     decelerates and runs in the negative direction at low speed. When it encounters the falling edge of the origin,
     the position of the first Z pulse is the origin position.
                                             负限位
                                              Negative                   原点开关
                                                                          Origin switch
                                               limit

                                                                                ①

                                                                        ②
                                                   ③



                                    Z信号
                                Z signal
                                    原点
                                 Origin
                        Negative负限位
                                limit
                                                                                                                        -57-


---

<!-- PDF page 62 -->

EtherCAT 总线伺服驱动器用户手册
6098=17: Reference negative limit return-to-origin mode
Situation 1: The negative limit signal is invalid when starting to return to zero. The axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the negative limit, the motor
         decelerates and runs in the forward direction. When encountering the falling edge of the negative limit, the
         motor decelerates and runs in the forward direction. The position at time is the origin position.
Situation 2: When starting the zero return, the negative limit signal is valid, and the axis starts to return to zero at a
         low speed in the positive direction. The position when it encounters the negative limit falling edge is the
         origin position.
                                                         负限位
                                                          Negative limit



                                                                                 ①

                                            ②
                      Negative limit
                               负限位


6098=18: Reference positive limit return-to-origin mode
Situation 1: The positive limit signal is invalid when starting to return to zero. The axis starts to return to zero in the
         positive direction at high speed. When encountering the rising edge of the positive limit, the motor
         decelerates and runs in the negative direction. When encountering the falling edge of the positive limit, the
         motor decelerates and runs in the negative direction. The position at time is the origin position.
Situation 2: When starting the zero return, the positive limit signal is valid, and the axis starts to return to zero in the
         negative direction at low speed. The position when it encounters the falling edge of the positive limit is the
         origin position.
                                                            Positive正限位
                                                                     limit



                                             ①

                                                                                       ②

                      Positive正限位
                               limit
6098=19: Reference origin switch return-to-origin mode 1
Situation 1: The origin signal is invalid when starting to return to zero. The axis starts to return to zero in the positive
         direction at high speed. When encountering the rising edge of the origin, the motor decelerates and runs in
         the negative direction. The position when encountering the falling edge of the origin is the origin position. .
Situation 2: When the origin signal is valid when starting the zero return, the axis starts to return to zero at a low
         speed in the negative direction. The position when it encounters the falling edge of the origin is the origin
         position.
                                         原点开关
                                        Origin switch




                                        ①

                                                              ②

                                原点
                              Origin


    -58-


---

<!-- PDF page 63 -->

EtherCAT 总线伺服驱动器用户手册
6098=20: Reference origin switch return-to-origin mode 2
Situation 1: The origin signal is invalid when the zero return is started. The axis starts to return to zero at a low speed
         in the forward direction. The position when it encounters the rising edge of the origin is the origin position.
Situation 2: When the origin signal is valid when the zero return is started, the axis starts to return to zero at a low
         speed in the negative direction. When encountering the falling edge of the origin, the motor decelerates and
         runs in the forward direction. When it encounters the rising edge of the origin, the position is the origin
         position.
                                        原点开关
                                       Origin switch




                                      ①
                                                           ②


                               原点
                             Origin

6098=21: Return to origin mode of reference origin switch
Situation 1: The origin signal is invalid when the zero return is started. The axis starts to return to zero at high speed
         in the negative direction. When it encounters the rising edge of the origin, the motor decelerates and runs in
         the forward direction. When it encounters the falling edge of the origin, the position is the origin position. .
Situation 2: When the origin signal is valid when the zero return is started, the axis starts to return to zero at a low
         speed in the forward direction. The position when it encounters the falling edge of the origin is the origin
         position.

                                                                         原点开关
                                                                        Origin switch




                                                                                ①

                                                            ②



6098=22: Reference origin switch return-to-origin mode
Scenario 1: The origin signal is invalid when the zero return is started. The axis starts to return to zero at a low speed
         in the negative direction. The position when it encounters the rising edge of the origin is the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis starts to return to zero at a low
         speed in the positive direction. When it encounters the falling edge of the origin, the motor decelerates and
         runs in the negative direction. When it encounters the rising edge of the origin, the position is the origin
         position.
                                                                         Origin
                                                                          原点开关  switch




                                                                                    ①
                                                                ②




                                                                                                                       -59-


---

<!-- PDF page 64 -->

EtherCAT 总线伺服驱动器用户手册
6098=23: Reference origin switch and positive limit return-to-origin mode
Situation 1: When starting the zero return, the origin signal is invalid. The axis starts to return to zero at high speed in
         the positive direction. When encountering the rising edge of the origin, the motor decelerates and runs in
         the negative direction at low speed. When it encounters the falling edge of the origin, the position is origin
         position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis starts to return to zero at a low
         speed in the negative direction. When it encounters the falling edge of the origin, it is the origin position.
Situation 3: When starting the zero return, the origin signal is invalid. The axis starts to return to zero at high speed in
         the positive direction. When encountering the rising edge of the positive limit, the motor decelerates and
         runs in the negative direction at high speed. When encountering the rising edge of the origin, , the motor
         decelerates and runs in the negative direction at low speed. The position when it encounters the falling
         edge of the origin is the origin position.
                                        原点开关
                                    Origin switch                              正限位
                                                                        Positive limit



                                           ①

                                                           ②                 ③


                               原点
                              Origin

                             正限位
                      Positive limit
6098=24: Reference origin switch and positive limit return-to-origin mode
Situation 1: When starting the zero return, the origin signal is invalid. The axis starts to return to zero at high speed in
         the positive direction. When encountering the rising edge of the origin, the motor decelerates and runs in
         the negative direction at low speed. When encountering the falling edge of the origin, the motor The
         deceleration reverse direction runs in the forward direction at low speed. The position after encountering the
         rising edge of the origin is the origin position.
Situation 2: When the zero return starts, the origin signal is valid, and the axis starts to return to zero at a low speed
         in the negative direction. When encountering the falling edge of the origin, the motor decelerates and runs
         in the positive direction at low speed. When it encounters the rising edge of the origin, the position is origin
         position.
Situation 3: When starting the zero return, the origin signal is invalid. The axis starts to return to zero at high speed in
         the positive direction. When encountering the rising edge of the positive limit, the motor decelerates and
         runs in the negative direction at high speed. When encountering the rising edge of the origin, , the motor
         decelerates and runs at low speed. When it encounters the falling edge of the origin, the motor decelerates
         and runs in the forward direction at low speed. When it encounters the rising edge of the origin, the position
         is the origin position.
                                              原点开关
                                           Origin switch                       正限位
                                                                        Positive limit



                                           ①


                                                               ②
                                                                             ③



                                原点
                              Origin

                              正限位
                       Positive limit
    -60-


---

<!-- PDF page 65 -->

EtherCAT 总线伺服驱动器用户手册
6098=25: Reference origin switch and positive limit return-to-origin mode
Situation 1: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero in the
         forward direction at high speed. When encountering the rising edge of the origin, the motor decelerates and
         runs forward at low speed. After encountering the falling edge of the origin, the motor reverses and runs at
         low speed. Running in the negative direction, the position when encountering the rising edge of the origin is
         the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis starts to return to zero at a low
         speed in the positive direction. When encountering the falling edge of the origin, the motor decelerates and
         runs in the negative direction at low speed. The position where it encounters the rising edge of the origin is
         the origin. Location.
Situation 3: When starting the zero return, the origin signal is invalid. The axis starts to return to zero at high speed in
         the positive direction. When encountering the rising edge of the positive limit, the motor decelerates and
         runs in the negative direction at high speed. After encountering the rising edge of the origin, the motor The
         deceleration reverse direction moves in the positive direction at a low speed. When it encounters the falling
         edge of the origin, it reverses and then moves in the negative direction at a low speed. The position when it
         encounters the rising edge of the origin is the origin position.

                                     原点开关
                                    Origin switch                      Positive正限位
                                                                                limit



                                           ①

                                                    ②
                                                                             ③




                               原点
                              Origin

                              正限位
                       Positive limit

6098=26: Reference origin switch and positive limit return-to-origin mode
Situation 1: The origin signal is invalid when starting to return to zero. The axis starts to return to zero in the forward
   direction at high speed. When encountering the rising edge of the origin, the motor decelerates and runs at low
   speed. When encountering the falling edge of the origin, the position is the origin position.
Situation 2: When the origin signal is valid when the zero return is started, the axis starts to return to zero at a low
   speed in the forward direction. The position when it encounters the falling edge of the origin is the origin position.
Situation 3: The origin signal is invalid when starting to return to zero. The axis starts to return to zero in the positive
   direction at high speed. When encountering the rising edge of the positive limit, the motor decelerates and runs in
   the negative direction at high speed. When encountering the rising edge of the origin opening, When , the motor
   decelerates and runs in the forward direction at low speed. When it encounters the falling edge of the origin, the
   position is the origin position.

                                        Origin switch
                                            原点开关                    Positive正限位
                                                                            limit



                                             ①

                                                     ②
                                                                           ③



                                    原点
                                 Origin

                          Positive正限位
                                  limit
                                                                                                                        -61-


---

<!-- PDF page 66 -->

EtherCAT 总线伺服驱动器用户手册
6098=27: Reference origin switch and negative limit return-to-origin mode
Situation 1: When starting to return to zero, the origin signal is invalid. The axis starts to return to zero at high speed
         in the negative direction. When encountering the rising edge of the origin, the motor decelerates and runs in
         the forward direction at low speed. When encountering the falling edge of the origin, the position is origin
         position.
Situation 2: When the origin signal is valid when starting the zero return, the axis starts to return to zero in the forward
         direction at a low speed. When it encounters the falling edge of the origin, it is the origin position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When it encounters the rising edge of the negative limit, the motor
         decelerates and runs in the positive direction at high speed. When it encounters the rising edge of the
         origin, , the motor decelerates and runs at low speed. When it encounters the falling edge of the origin, the
         position is the origin position.
                                             Negative limit
                                            负限位                               Origin switch
                                                                             原点开关



                                                                                  ①

                                                   ③                ②

                                   原点
                                Origin

                                负限位
                         Negative limit

6098=28: Reference origin switch and negative limit return-to-origin mode
Situation 1: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the origin, the motor decelerates and
         runs in the forward direction at low speed. When encountering the falling edge of the origin, the motor The
         deceleration reverse direction runs in the negative direction at low speed. When it encounters the rising
         edge of the origin, the position is the origin position.
Situation 2: When the origin signal is valid when starting the zero return, the axis starts to return to zero at a low
         speed in the positive direction. When encountering the falling edge of the origin, the motor decelerates and
         runs in the negative direction at a low speed. When it reaches the rising edge of the origin, it is the origin
         position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the negative limit, the motor
         decelerates and moves in the positive direction at high speed. When encountering the rising edge of the
         origin, , the motor decelerates and runs at low speed. When it encounters the falling edge of the origin, the
         motor decelerates and runs in the negative direction at low speed. When it encounters the rising edge of the
         origin, it is the origin position.
                                              Negative limit
                                              负限位                       Origin
                                                                         原点开关  switch



                                                                                ①


                                                                ②
                                                   ③



                                    原点

                         Negative负限位
                                 limit



    -62-


---

<!-- PDF page 67 -->

EtherCAT 总线伺服驱动器用户手册
6098=29: Reference origin switch and negative limit return-to-origin mode
Situation 1: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the origin, the motor decelerates and
         runs at low speed. When encountering the falling edge of the origin, the motor decelerates and reverses to
         low speed. Running in the forward direction, the position when encountering the rising edge of the origin is
         the origin position.
Situation 2: When the zero return starts, the origin signal is valid, and the axis starts to return to zero at a low speed
         in the negative direction. When encountering the falling edge of the origin, the motor decelerates and runs
         in the positive direction at low speed. When it encounters the rising edge of the origin, the position is origin
         position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When it encounters the rising edge of the negative limit, the motor
         decelerates and runs in the positive direction at high speed. When it encounters the rising edge of the
         origin, , the motor decelerates and runs in the negative direction at low speed. When it encounters the
         falling edge of the origin, the motor decelerates and runs in the forward direction at low speed. When it
         encounters the rising edge of the origin, the position is the origin position.
                                               Negative limit
                                              负限位                          Origin switch
                                                                           原点开关



                                                                              ①

                                                                       ②
                                                   ③




                                     原点
                                  Origin
                          Negative负限位
                                  limit


6098=30: Reference origin switch and negative limit return-to-origin mode
Situation 1: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When encountering the rising edge of the origin, the motor decelerates and
         runs at low speed. When encountering the falling edge of the origin, the position is the origin position.
Situation 2: When the zero return is started, the origin signal is valid, and the axis starts to return to zero at a low
         speed in the negative direction. When it encounters the falling edge of the origin, it is the origin position.
Situation 3: When starting the zero return, the origin signal is invalid, and the axis starts to return to zero at high
         speed in the negative direction. When it encounters the rising edge of the negative limit, the motor
         decelerates and runs in the positive direction at high speed. When it encounters the rising edge of the
         origin, , the motor decelerates and runs in the negative direction at low speed. When it encounters the
         falling edge of the origin, the position is the origin position.

                                         负限位
                                         Negative limit             原点开关
                                                                    Origin switch




                                                                                    ①

                                                                   ②
                                               ③



                             原点
                            Origin

                     Negative limit
                           负限位
                                                                                                                      -63-


---

<!-- PDF page 68 -->

EtherCAT 总线伺服驱动器用户手册
6098=33/34: Reference Z signal return-to-origin mode
Zero return method 33: The axis starts to return to zero in the negative direction at low speed. The position of the first
         Z pulse encountered is the origin position.
Zero return mode 34: The axis starts to return to zero in the forward direction at low speed, and the position of the
         first Z pulse encountered is the origin position.




                                                               33
                                                            34

                             Z signal
                                 Z信号

6098=35: Take the current position as the origin
Taking the current position as the mechanical origin, after triggering the origin return (6040 control word: 0x0F→
         0x1F):
1. When 60E6=0, set the current position 6064 to the value of the origin offset 606C;
2. When 60E6=1, the current position 6064 is superimposed on the original position offset 606C.




    -64-
