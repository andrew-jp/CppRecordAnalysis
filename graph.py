#graph.py

from matplotlib import pyplot as pp
import os

def main():
    with open(".\smoothdata.txt") as file:
        smdata = file.read().split()

    pp.plot(smdata)
    pp.show()

if __name__ == "__main__":
    main()