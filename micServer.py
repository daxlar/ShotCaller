import socket
import threading
import errno
import time
import requests

shot_state = 0

left = 0
middle = 1
right = 2
microphones = [left, middle, right]

frequency = 0
magnitude = 1
phase = 2
amplitude = 3
left_mic_data = [0]*4
middle_mic_data = [0]*4
right_mic_data = [0] * 4

connectionCount = 0
address = 0
port = 55555
esp8266socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
hostName = socket.gethostname()
hostIP = socket.gethostbyname(hostName)

print(hostIP)
esp8266socket.bind((hostIP, port))
esp8266socket.listen(5)
print('im here')


def email_alert(sector):
    # requests.post("https://maker.ifttt.com/trigger/gunshot_detected/with/key/dCOToeQZzXJRGlzaWT6c5O?value1={}".format(sector))
    print("gunshot detected in the ")
    print(sector)
    print("sector")
    left_mic_data[frequency] = 0
    right_mic_data[frequency] = 0
    middle_mic_data[frequency] = 0


def read(microphoneLocation, connection):
    global shot_state
    # another option is to make it blocking and remove the exception handling
    connection.setblocking(0)
    while True:
        try:
            # due to non-blocking, if the packet is not received, raises exception
            # therefore, repeated prints are not executed because no packet = no print
            packet = connection.recv(4)
            data_array = list(packet)
            if microphoneLocation == 'leftMicrophone':
                # print('left')
                # print(data_array)
                shot_state = 1
                left_mic_data[frequency] = int.from_bytes(data_array[0:1], byteorder='big') * 255
                left_mic_data[magnitude] = int.from_bytes(data_array[1:2], byteorder='big') * 255
                left_mic_data[phase] = int.from_bytes(data_array[2:3], byteorder='big') * 1
                left_mic_data[amplitude] = int.from_bytes(data_array[3:4], byteorder='big') * 255
            if microphoneLocation == 'middleMicrophone':
                # print('middle')
                # print(data_array)
                shot_state = 1
                middle_mic_data[frequency] = int.from_bytes(data_array[0:1], byteorder='big') * 255
                middle_mic_data[magnitude] = int.from_bytes(data_array[1:2], byteorder='big') * 255
                middle_mic_data[phase] = int.from_bytes(data_array[2:3], byteorder='big') * 1
                middle_mic_data[amplitude] = int.from_bytes(data_array[3:4], byteorder='big') * 255
            if microphoneLocation == 'rightMicrophone':
                # print('right')
                # print(data_array)
                shot_state = 1
                right_mic_data[frequency] = int.from_bytes(data_array[0:1], byteorder='big') * 255
                right_mic_data[magnitude] = int.from_bytes(data_array[1:2], byteorder='big') * 255
                right_mic_data[phase] = int.from_bytes(data_array[2:3], byteorder='big') * 1
                right_mic_data[amplitude] = int.from_bytes(data_array[3:4], byteorder='big') * 255
        except socket.error as e:
            if e.args[0] == errno.EWOULDBLOCK:
                # print('blocked')
                time.sleep(0.1)


while connectionCount != 3:
    print('connection is really slow')
    connection, address = esp8266socket.accept()
    print('connected to ', address)

    if address[0] == '192.168.43.198':
        microphones[left] = threading.Thread(target=read, args=('leftMicrophone', connection,))
        connectionCount = connectionCount + 1

    if address[0] == '192.168.43.143':
        microphones[middle] = threading.Thread(target=read, args=('middleMicrophone', connection,))
        connectionCount = connectionCount + 1

    if address[0] == '192.168.43.192':
        microphones[right] = threading.Thread(target=read, args=('rightMicrophone', connection,))
        connectionCount = connectionCount + 1


microphones[left].start()
microphones[middle].start()
microphones[right].start()

# logic goes like this :
# first check to see which microphones have the same frequency content
# compare their magnitudes
# if they are the same, then check the phase
# if they are the same, check the max amplitude
# amplitude can be used to check the depth of the gunshot

while True:
    if left_mic_data[frequency] == middle_mic_data[frequency] == right_mic_data[frequency] :
        # check to see if initial state
        if left_mic_data[frequency] == 0:
            left = left % 1
        elif (middle_mic_data[magnitude] > left_mic_data[magnitude]) and (middle_mic_data[magnitude] > right_mic_data[magnitude]):
            # print('middle')
            email_alert('middle')
        elif (left_mic_data[magnitude] > middle_mic_data[magnitude]) and (left_mic_data[magnitude] > right_mic_data[magnitude]):
            # print('left')
            email_alert('left')
        else:
            # print('right')
            email_alert('right')
    elif left_mic_data[frequency] == middle_mic_data[frequency]:
        if (right_mic_data[frequency] + 1 == middle_mic_data[frequency]) or (right_mic_data[frequency] - 1 == middle_mic_data[frequency]):
            if (right_mic_data[magnitude] > middle_mic_data[magnitude]) and (right_mic_data[magnitude] > left_mic_data[magnitude]):
                # print('right')
                email_alert('right')
        elif left_mic_data[magnitude] > middle_mic_data[magnitude]:
            # print('left')
            email_alert('left')
        elif middle_mic_data[magnitude] > left_mic_data[magnitude]:
            # print('middle')
            email_alert('middle')
        else:
            if left_mic_data[phase] > middle_mic_data[phase]:
                # print('middle')
                email_alert('middle')
            elif middle_mic_data[phase] > left_mic_data[phase]:
                # print('left')
                email_alert('left')
            else:
                if left_mic_data[amplitude] > middle_mic_data[amplitude]:
                    # print('left')
                    email_alert('left')
                else:
                    # print('middle')
                    email_alert('middle')
    elif right_mic_data[frequency] == middle_mic_data[frequency]:
        if (left_mic_data[frequency] + 1 == middle_mic_data[frequency]) or (left_mic_data[frequency] - 1 == middle_mic_data[frequency]):
            if (left_mic_data[magnitude] > middle_mic_data[magnitude]) and (left_mic_data[magnitude] > right_mic_data[magnitude]):
                # print('left')
                email_alert('left')
        elif right_mic_data[magnitude] > middle_mic_data[magnitude]:
            # print('right')
            email_alert('right')
        elif middle_mic_data[magnitude] > right_mic_data[magnitude]:
            # print('middle')
            email_alert('middle')
        else:
            if right_mic_data[phase] > middle_mic_data[phase]:
                # print('middle')
                email_alert('middle')
            elif middle_mic_data[phase] > right_mic_data[phase]:
                # print('right')
                email_alert('right')
            else:
                if right_mic_data[amplitude] > middle_mic_data[amplitude]:
                    # print('right')
                    email_alert('right')
                else:
                    # print('middle')
                    email_alert('middle')
    else:
        if(middle_mic_data[magnitude] > left_mic_data[magnitude]) and (middle_mic_data[magnitude] > right_mic_data[magnitude]):
            # print('middle last')
            email_alert('middle')
        elif (left_mic_data[magnitude] > middle_mic_data[magnitude]) and (left_mic_data[magnitude] > right_mic_data[magnitude]):
            # print('left last')
            email_alert('left')
        else:
            # print('right last')'
            email_alert('right')

    """
    # very problematic with multi-threading
    if shot_state == 1:
        left_mic_data[frequency] = 0
        right_mic_data[frequency] = 0
        middle_mic_data[frequency] = 0
        shot_state = 0
    """

