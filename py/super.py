import serial
import time

def processline(str, port):
    x = str.split(';')
    print(str, end='')
    port.write(bytearray(str, 'ascii'))
    time.sleep(0.02)


def main():
    print("Hello\r\n")
    ser = serial.Serial("/dev/ttyACM0", 460800)
    ser.write(b'[begin:108]\n')
    wi = 120
    with open("py/waveform-0.cvs_wf", 'r') as f:
        while True:
            line = f.readline()
            # wi = wi - 1
            if wi == 0:
                break
            if not line:
                break
            processline(line, ser)
        
        ser.write(b'[row:0]\n')
        time.sleep(3)
        ser.write(b'[start:1000]\n')
'''
Если текущий файл запущен из консоли, вызывается main(), если подгружен как import - не вызывается
'''
if __name__=="__main__":
    main()


