import time
import socket
import pyautogui
import threading
from queue import Queue

class InMessage:
    def __init__(self, data: str, require_ack: bool, client_addr: str) -> None:
        self.data = data
        self.require_acknowledgment = require_ack
        self.client_addr = client_addr

class OutMessage:
    def __init__(self, data: str, require_ack: bool = False) -> None:
        self.data = data
        print(data)

class Socket:
    ACKNOWLEDGMENT_FLAG = 'A'
    CPU_RELEASE_SLEEP = 0.000_001

    def __init__(self, max_buffer_sz: int, port: int = 11111, in_queue_sz: int = 0, out_queue_sz: int = 0) -> None:
        assert max_buffer_sz > 0, f"Buffer size must be > 0 [{max_buffer_sz = }]"
        assert in_queue_sz >= 0, f"Queue size can't be negative [{in_queue_sz = }]"
        assert out_queue_sz >= 0, f"Queue size can't be negative [{out_queue_sz = }]"

        self._rip = False
        self._have_client = False
        self._max_buffer_size = max_buffer_sz
        self._incoming_messages_queue = Queue(maxsize=in_queue_sz)
        self._outgoing_messages_queue = Queue(maxsize=out_queue_sz)

        self._client = None
        self._client_address = None
        soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        soc.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        soc.bind(('0.0.0.0', port))
        soc.listen(0)

        self._threads = [
            threading.Thread(target=self.__listener_thread, daemon=True),
            threading.Thread(target=self.__sender_thread, daemon=True),
            threading.Thread(target=self.__wait_for_connection_thread, daemon=True, args=[soc]),
        ]
        for thread in self._threads:
            thread.start()

    def auto_click(self, x, y, clicks=1, interval=0.0):
        pyautogui.click(x, y, clicks=clicks, interval=interval)
    
    def get_message(self) -> InMessage:
        return self._incoming_messages_queue.get()

    def send_message(self, message: OutMessage) -> None:
        self._outgoing_messages_queue.put(message)
        print(self._outgoing_messages_queue.put(message))

    def destroy(self):
        if self._client is not None:
            self._client.close()
        
        self._rip = True
        for thread in self._threads:
            thread.join(0.1)

    def __wait_for_connection_thread(self, soc: socket.socket) -> None:
        self._client, self._client_address = soc.accept()
        self._have_client = True

    def __decode(self, in_bytes: bytes) -> 'None|InMessage':
        message = in_bytes.decode()
        if not len(message):
            return None
        ack = message[0] == self.ACKNOWLEDGMENT_FLAG
        data = message[1 * ack:]
        return InMessage(data=data, require_ack=ack, client_addr=self._client_address)
        
    def __listener_thread(self):
        while not self._rip:
            if not self._have_client:
                time.sleep(self.CPU_RELEASE_SLEEP)
                continue

            message = self._client.recv(self._max_buffer_size)
            decoded_msg = self.__decode(message)
            
            if decoded_msg is not None:
                self._incoming_messages_queue.put(decoded_msg)
                call = pyautogui.locateCenterOnScreen("C:/Users/AlexisLapraille/Downloads/ESP32_Python_WiFi-master/ESP32_Python_WiFi-master/scripts/call2.png")
                print("......")
                print("Message serveur : La connection à bien été établie avec le client")

                if call is not None:
                    pyautogui.moveTo(call)
                    self.auto_click(call[0], call[1])
                    callValidation_start = self.__encode(OutMessage(data='n'))
                    self._client.send(callValidation_start)
                    time.sleep(3)
                    callValidation_stop = self.__encode(OutMessage(data='f'))
                    self._client.send(callValidation_stop)
                    full_screen = pyautogui.locateCenterOnScreen("C:/Users/AlexisLapraille/Downloads/ESP32_Python_WiFi-master/ESP32_Python_WiFi-master/scripts/full_screen.png")
                    pyautogui.moveTo(full_screen)
                    self.auto_click(full_screen[0], full_screen[1])

                else:
                    print("")
                    callFailed_start = self.__encode(OutMessage(data='h'))
                    self._client.send(callFailed_start)
                    time.sleep(2)
                    callFailed_stop = self.__encode(OutMessage(data='c'))
                    self._client.send(callFailed_stop)


    def __encode(self, message: OutMessage) -> bytes:
        return message.data.encode()

    def __sender_thread(self):
        while not self._rip:
            if not self._have_client:
                time.sleep(self.CPU_RELEASE_SLEEP)
                continue

            msg = self._outgoing_messages_queue.get()
            self._client.send(self.__encode(msg))
