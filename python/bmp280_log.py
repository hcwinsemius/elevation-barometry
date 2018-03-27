import os
import serial
import serial.tools.list_ports
import datetime
import numpy as np
import pandas as pd
import time

def find_arduino():
    """
    Looks for an Arduino in all active serial ports
    TODO: check if this function works appropriately on a linux box.
    :return: str - name of COM port (e.g. 'COM1')

    """
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        port = p[0]
        description = p[1]
        if 'Arduino' in description:
            return port
    # if the end of function is reached, apparently, no suitable port was found
    print('No Arduino found on COM ports')
    return None

def add_line(df, data, cols):
    """
    Adds a line to an existing pd.DataFrame and returns appended DataFrame. The current time is added as index
    :param df: pd.DataFrame
    :param data: comma separated set of values (3, pressure, temperature and elevation)
    :param cols: column names
    :return: pd.DataFrame (appended)
    """
    # estimate current time (of observation)
    time = np.datetime64(datetime.datetime.now())
    # Make a one-line _DataFrame
    _data = pd.DataFrame(np.array([[float(n) for n in data.split(',')]]), columns=cols)
    # set the index name of _DataFrame
    _data.index = [time]
    # append DataFrame to DataFrame
    return df.append(_data)

def create_log_filename(path, prefix):
    """
    Make a well-formatted log filename using a path and prefix.
    Filename is <path>/<prefix>_<YYYYMMDDTHHMMSS>.csv with current time as timestr.
    :param path: str - path to write file in
    :param prefix: str - prefix of filename
    :return: str - filename inc. path
    """
    return os.path.join(path, '{:s}_{:s}.csv'.format(prefix, datetime.datetime.now().strftime('%Y%m%dT%H%M%S')))

def read_bmp280(ser, duration=None, skiplines=0, header_contains=None):
    """
    Reads BMP280 on Arduino from a serial connection, assuming the first line is a message to start
    monitoring, the second line is the header, and all lines below are 3 comma separated values (pressure, temperature
    and elevation). Returns a pandas dataframe with a time index
    :param ser:
    :param duration=None: number of seconds to read. If set to None, it will read as long as the serial port provides
        data
    :return: df
    """
    if header_contains is not None:
        while True:
        #     data = ser.readline()
            data = ser.readline()
        #     string = data.decode('utf-8')
            if header_contains in str(data):
                header = data
                break
        
    else:
        for l in range(skiplines):
        # skip a defined set of lines
            ser.readline()
        header = ser.readline()
        
    # start with an empty DataFrame
    header = header.decode('utf-8').rstrip().split(', ')
    print(header)
    df = pd.DataFrame()
    print('Reading from {:s}'.format(ser.port))
    print('If you want to stop reading, please disconnect or reset the Arduino')
    begin_time = time.time()
    # read as long as no empties are returned or time does not exceed duration
    while not(time.time() - begin_time > duration):
        # ask for a line of data from the serial port
        data = ser.readline()
    #     print data
        if data == '':
            print('No data received from Arduino for {:f} seconds, returning data now!')
            break
        try:
            # add a line using the read data
            df = add_line(df, data.decode('utf-8'), cols=header)
        except:
            print('Received an incorrectly formatted line from {:s}. Did you upload the right sketch?'.format(ser.port))
            print(data.decode('utf-8'))
            break
    # rename the index name to time and return the pd.DataFrame
    df.index.name = 'time'
    return df

def log_bmp280(path, prefix, timeout=5, duration=1e9, write_csv=True, baud_rate=9600, skiplines=0, header_contains=None):
    """
    Logs comma separated values from Arduino connected BMP280, assuming the first line is a message to start
    monitoring, the second line is the header, and all lines below are 3 comma separated values (pressure, temperature
    and elevation). Data is automatically written to a .csv file with given path, prefix and (current) time stamp

    :param path: str - path to output log file
    :param prefix: str - prefix to output log file
    :param timeout=5: int - function stops reading when serial port is quiet for <timeout> seconds
    :param duration=None: int - if set to a number, functions stops reading when it has read for <duration> seconds
    :param write_csv=True: bool - if set, a csv file with results is written to <path>/<prefix>_<yyyymmddTHHMMSS>.csv
    :param baud_rate: int - baud rate to read serial port with
    :param skiplines: int - amount of lines to skip before reaching the comma-separated header
    :return: df (and a written file if write_csv set to True
    """
    fn = create_log_filename(path, prefix)
    port = find_arduino()
    if port is None:
        print('Cannot read from Arduino')
        return None
    ser = serial.Serial(port, baud_rate, timeout=timeout)

    if not(os.path.isdir(path)):
        os.makedirs(path)
    # read the pd.DataFrame, disconnect or reset Arduino to stop this function and return a pd.DataFrame
    df = read_bmp280(ser, skiplines=skiplines, duration=duration, header_contains=header_contains)
    # close connection and write pd.DataFrame to file
    ser.close()
    # write pd.DataFrame object to file
    df.to_csv(fn)
    return df
