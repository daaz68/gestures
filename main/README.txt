Shunt voltage input range: -81.92 +81.92
The resolution of the shunt voltage is 2.5 mV with a full scale of 32768x2.5 = 81.92mV.
For the VBUS voltage the resolution is 1.25 mV with a theoretical full-scale of 40.96 V even if the 36 V must not be exceeded.
The resolution of the power is 25 times that of the current, with a full-scale that depends on the shunt used. So the system has a remarkable measurement accuracy.

Measureable current Ifs = 81.92mV / 100mOhm = 0.800A (800mA)
Current resolution 0.8A / 32768 = 2.44e-5A or 0.0244mA

Rshunt = 0.1ohm
Imax = 1A

currentLSB = Imax / 32768 = 3e-5A
calibration = 0.00512 / currentLSB / Rshunt = 0.00512 / 3e-5A / 0.1ohm = 1706
powerLSB = currentLSB * 25 = 7.5e-4W
