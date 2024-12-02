import zipfile
import os
import csv
import time
from datetime import datetime



# Get the current date and time
current_time = datetime.now()
current_day = current_time.strftime("%A")
current_date = current_time.strftime('%Y-%m-%d')
formatted_time = current_time.strftime('%H-%M-%S')  # Use '-' to avoid file system issues with ':'

# Get the modified time of the file
modified_time = os.path.getmtime('src/main.cpp')
modified_datetime = datetime.fromtimestamp(modified_time)
modified_time_str = modified_datetime.strftime('%Y-%m-%d %H:%M:%S')
modified_time_str = modified_time_str.split(' ')
# Name of the resulting zip file
zip_name = f'backup/backup_{current_day}_{current_date}_{formatted_time}.zip'

files = ['src/main.cpp']

# Create the zip file
def create_zip(zip_filename, files_to_zip):
    # Create a new zip file
    with zipfile.ZipFile(zip_filename, 'w') as zipf:
        for file in files_to_zip:
            # Add each file to the zip file
            zipf.write(file, os.path.basename(file))

        # Write the file path and modified time to a CSV file
    with open('backup/filesPath.csv', 'w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        for file in files:
            csv_writer.writerow(modified_time_str)


def setModifiedFileName():
    # List of files you want to zip
    
    # create_zip(zip_name, files)
    timechecker =''
    print(f"Created {zip_name} successfully.")
    with open('backup/filesPath.csv', 'r', newline='') as csvfile:
        csv_writer = csvfile.read()
        timechecker = csv_writer.split(',')
        print('here',timechecker[1],modified_time_str[1])
    modhr,modmin,modsec = timechecker[1].split(':')
    cherhr,chermin,chersec = modified_time_str[1].split(':')
    print(cherhr,modmin)
    if(int(chermin) > int(modmin)):
        print('modification loop started')
        create_zip(zip_name, files)
# Run the function
setModifiedFileName()
