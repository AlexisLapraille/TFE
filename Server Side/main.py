from gui import GUI
from socket import Socket

def main():
    tunnel = Socket(max_buffer_sz=128)
    gui = GUI(tunnel=tunnel)
    gui.mainloop()

if __name__ == '__main__':
    main()
