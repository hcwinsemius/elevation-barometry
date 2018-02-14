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

def read_bmp280(ser, duration=None):
    """
    Reads BMP280 on Arduino from a serial connection, assuming the first line is a message to start
    monitoring, the second line is the header, and all lines below are 3 comma separated values (pressure, temperature
    and elevation). Returns a pandas dataframe with a time index
    :param ser:
    :param duration=None: number of seconds to read. If set to None, it will read as long as the serial port provides
        data
    :return: df
    """
    print ser.readline()
    # start with an empty DataFrame
    df = pd.DataFrame()
    header = ser.readline().rstrip().split(', ')[1:]
    print header
    print('Reading from {:s}'.format(ser.port))
    print('If you want to stop reading and write an excel sheet, please disconnect or reset the Arduino')
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
            df = add_line(df, data, cols=header)
        except:
            print('Received an incorrectly formatted line from {:s}. Did you upload the right sketch?'.format(ser.port))
            break
    # rename the index name to time and return the pd.DataFrame
    df.index.name = 'time'
    return df

def log_bmp280(path, prefix, timeout=5, duration=None, write_csv=True):
    """
    Logs comma separated values from Arduino connected BMP280, assuming the first line is a message to start
    monitoring, the second line is the header, and all lines below are 3 comma separated values (pressure, temperature
    and elevation). Data is automatically written to a .csv file with given path, prefix and (current) time stamp

    :param path: str - path to output log file
    :param prefix: str - prefix to output log file
    :param timeout: str - timeout, function stops reading when timeout is reached
    :return: None (a file with logged values is written instead)
    """
    fn = create_log_filename(path, prefix)
    port = find_arduino()
    if port is None:
        print('Cannot read from Arduino')
        return None
    ser = serial.Serial(port, 9600, timeout=timeout)

    if not(os.path.isdir(path)):
        os.makedirs(path)
    # read the pd.DataFrame, disconnect or reset Arduino to stop this function and return a pd.DataFrame
    df = read_bmp280(ser, duration=duration)
    # close connection and write pd.DataFrame to file
    ser.close()
    # write pd.DataFrame object to file
    df.to_csv(fn)
    return df
