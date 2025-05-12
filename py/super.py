import serial
import time

def processline(str, port):
    x = str.split(';')
    print(str, end='')
    port.write(bytearray(str, 'ascii'))
    time.sleep(0.1)


def main():
    print("Hello\r\n")
    ser = serial.Serial("COM4", 115200)
    ser.write(b'[begin]\n')
    wi = 120
    with open("e:\\!git\\waveform-0.cvs_wf", 'r') as f:
        while True:
            line = f.readline()
            wi = wi - 1
            if wi == 0:
                break
            if not line:
                break
            processline(line, ser)

main()


