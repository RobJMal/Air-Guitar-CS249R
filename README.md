# Air-Guitar-CS249R

Welcome to Air Guitar CS249R! This Repo contains the code, resources and supporting 
materials for the Air Guitar final project for the class CS249R: Machine Learning!

At a high level, this project is concerned with two different machine learning models 
running on the Teensy 4.1: one project classifies gesture IMU data into two classes:
strum or no strum, while another model is concerned with classifying data from 
flex sensors attached to four fingers into 13 different chords.


The repo is organized as follows:

The base folder contains supporting information and resources, including the schematic for the
circuit used. 

The audio files folder includes the audio files used for the project. Specfically, the chord_recordings
subdirectory holds all the audio files played back upon playing the corresponding chord. The files 
are named with the name of the associated chord and all of them are .wav format.

The data directory holds all the data collected for this project used for training and validation.
The flex subdirectory holds the data from flex sensors, while the imu subdirectory holds the data
from the IMU. All data are stored in .csv format as time series data. All data belonging to one class
are stored in one .csv file with the name of the associated class, and separate data points are separated
by empty rows.

The data_collection directory holds all the scripts used to collect data for both models. There are two 
subdirectories, one for each of the models. Both of them include a python script as well as a Platform.IO
project. The python script includes further instructions in the top as comments.

The deployment directory contains the final and compiled source code of the project. It is a Platform.IO
project, and includes the header files for the models used. Most of the source code is included in the 
src subdirectory. The folder also contains the deployment.py script, which reads the serial data from the
Teensy running the deployment project, and plays the desired audio file. More detailed instuctions are at the
top of the python file.

The experimention folder contains multiple Arduino/Platform.IO projects that we used in our testing to interface
with the Teensy board.

The notebooks folders contains all the Jupyter Notebooks that we used to train our models. The folder contains
two notebooks for each sensor type, one for training a fully-connected model and another for training a convolutional
model.

Lastly, the results directory includes the results of all of our model training attempts for both sensors, including 
confusion matrices, as well as the TFLite files and the model header files. The two models that we ende up using 
for the deployment project were the flex-cnn-onlyonefclayer for the Flex sensor model and the strum-cnn-onlyonefclayer
for the IMU model.



## Chords Gestures

This project supports 13 chords. They are the following: A, A Minor, B, B Minor, C, C Minor, D, D Minor, E, E Minor, F, G, G Minor

Since it is hard to use the actual hand positions of these chords, both physically and for data collection and classification, we 
have come up with our gestures to represent these chords. Each gesture is composed of a combination of 4 fingers: index, middle, ring, 
and pinky, in that order. A 1 signifies that the finger is held straight, while a 0 signifies that a finger is bent towards the palm of the hand.
Using this encoding, the following are the gestures for each of the supported Chords:

a: 0001

am: 1001

b: 1000

bm: 1111

c: 0110

cm: 1011

d: 0010

dm: 0100

e: 0000

em: 1100

f: 1101

g: 1101

gm: 1010

## Hardware Design and Setup

This project uses sensors attached to gloves to collect the data and run the application. 
