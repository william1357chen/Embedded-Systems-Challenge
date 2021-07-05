# Here we don't need Matplotlib to Visualize data
import numpy as np 
from scipy.signal import find_peaks

# This function samples the deflation raw pressure data into a specific range
# Since we only need pressure from 175 - 60
def condense_deflation(array, low, high):
  for i in range(0, len(array)):
    if array[i] <= high:
      array = array[i:]
      break
  for i in range(len(array)-1, -1, -1):
    if array[i] >= low:
      array = array[:i]
      break
  return array

# This is the algorithm for calculating the moving average of a graph
def calculate_moving_average(array, interval):
    i = 0
    window_size = interval
    moving_averages = []
    while i < len(array) - window_size + 1:
        this_window = array[i : i + window_size]

        window_average = sum(this_window) / window_size
        moving_averages.append(window_average)
        i += 1  
    return moving_averages

# The function compare peak values and find the SBP and DBP's value
def find_nearest(array, value):
  array = np.asarray(array)
  idx = (np.abs(array-value)).argmin()
  return array[idx]

def main():
  """
    Here we get the needed pressure data and time per measurement in files
  """
  with open('time_per_measurement.txt','r')as f:
    time_per_measurement = float(f.read())

  with open('inflation.txt','r')as f:
      string = f.read()

  inflation = string.split()
  inflation = np.array([float(num) for num in inflation])

  with open('deflation.txt','r')as f:
      string = f.read()

  deflation = string.split()
  deflation = np.array([float(num) for num in deflation])

  """
    Here we process the data
  """
  # Get pressure from 175 - 60
  useful_deflation = condense_deflation(deflation,60, 175)

  # Find the moving average of the deflation with interval of 50
  interval = 50
  moving_average = calculate_moving_average(useful_deflation, interval)

  # Because we lose 50 data points for the moving average, we sample off 50 data points on our deflation data
  # Sample off the first 25 and last 25 points of the deflation data to match the moving average
  useful_deflation = useful_deflation[interval // 2:len(useful_deflation) - (interval // 2) + 1]

  # We get the difference between the deflation and moving average to get the oscillations
  oscillations = np.abs(np.array(useful_deflation) - np.array(moving_average))

  # Find the peaks indices of the osillations
  # Have parameters width = 3 and distance = 40 to make sure peaks aren't too close
  peak_indices, _ = find_peaks(oscillations, width = 3,distance=40)

  # Find the actual pressure value of the peaks
  peak_pressures = [oscillations[i] for i in peak_indices]

  # Note that the trough of oscillations is zero so the vertical height of the peak is the difference between peak and trough

  """
    Here we do the calculations for finding:
      1. Max Pulse
      2. SBP
      3. DBP
      4. Heart Rate
  """
  # Get the index of the maximum peak value in peak_indices array
  max_index_pressure = peak_pressures.index(max(peak_pressures))

  # This is the index of the maximum peak value in the oscillations array
  max_index_oscillation = peak_indices[max_index_pressure]

  # This is the actual maximum peak value 
  max_pulse = oscillations[max_index_oscillation]

  # Systolic Blood Pressure is 50% of the maximum peak value to the left of the max pulse
  systolic = max_pulse * 0.5
  # Diastolic Blood Pressure is 80% of the maximum peak value to the right of the max pulse
  diastolic = max_pulse * 0.8

  # This is the index of the SBP in the oscillations array
  sbp_ind = peak_indices[peak_pressures.index(find_nearest(peak_pressures[0:max_index_pressure], systolic))]

  # This is the index of the DBP in the oscillations array
  dbp_ind = peak_indices[peak_pressures.index(find_nearest(peak_pressures[max_index_pressure:], diastolic))]

  # Here we find the max pulse
  Max_Pulse = useful_deflation[max_index_oscillation]
  print("Pressure for Max Pulse (mmHg) is: {}".format(Max_Pulse))

  # Here we find the systolic blood pressure
  SBP = useful_deflation[sbp_ind]
  print("Systolic Blood Pressure (mmHg) is: {}".format(SBP))

  # Here we find the diaastolic blood pressure
  DBP = useful_deflation[dbp_ind]
  print("Diastolic Blood Pressure (mmHg) is: {}".format(DBP))

  """
  To calculate heart rate, we first find the number of measurements per oscillation
  We do that by dividing the total number of measurements with the total number of peaks.
  Next we times the number of measurements per oscillation with the time per measurements
  to get the time per oscillation. Flip that, and we get the oscillation per second, which 
  is the heart rate. 
  """
  num_measurements_per_oscillation = len(oscillations)/len(peak_indices)
  heart_rate_s = 1 / ((num_measurements_per_oscillation * time_per_measurement) / 1000)
  heart_rate = heart_rate_s * 60
  print("Heart Rate (beats/min) is: {}".format(heart_rate))


if __name__ == '__main__':
  main()