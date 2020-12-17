import math
import sys

# Run e.g. python3 bioprinter_thermistors.py test.h 10000 8300
#   for 10k thermistor, 8.3k series resistor

if __name__ == '__main__':
    with open(sys.argv[1], "w") as output_file:
        Rt0 = float(sys.argv[2])    # Baseline thermistor resistance
        RS = float(sys.argv[3])     # Series resistor

        # These numbers are intrinsic to Amphenol Thermometrics Material Type 1
        a = 3.3539438e-3
        b = 2.5646095e-4
        c = 2.5158166e-6
        d = 1.0503069e-7

        Vbits = [i / 1024 for i in range(1024)]         # Possible normalized voltages
        Rt = [V * RS / (1 - V) / Rt0 for V in Vbits]    # Normalized resistance as function of voltage
        TK = [1 / (a + b*math.log(R) + c*math.log(R)**2 + d*math.log(R)**3) if R != 0 else 999 for R in Rt]
        
        output_file.writelines([
            "#ifndef DUKE_BIOPRINTER_THERMISTOR_TABLE\n",
            "#define DUKE_BIOPRINTER_THERMISTOR_TABLE\n",
            "\n",
            "#include \"Marlin.h\"\n",
            "\n",
            "/* Maps analog input value to temperature in Celsius\n",
            "   Current setting: {:.1e} thermistor, {:.1e} series resistor */\n".format(Rt0, RS),
            "const static float bioprinter_thermistor_table[] PROGMEM = {\n",
        ])
        output_file.writelines([
            "    {:f},  // {:d}\n".format(T - 273.15, i) for i, T in enumerate(TK)
        ])
        output_file.writelines([
            "};\n",
            "\n",
            "#endif\n"
        ])
