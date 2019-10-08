# Use an official Python runtime as a parent image
FROM python:3.6-slim

# Set the working directory to /app
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY . /app

# Install any needed packages specified in requirements.txt
RUN pip install --trusted-host pypi.python.org -r requirements.txt

RUN apt-get update
RUN apt-get --assume-yes install net-tools
RUN apt-get --assume-yes install vim
RUN apt-get --assume-yes install sudo
# Make port 80 available to the world outside this container
EXPOSE 80

# Define environment variable
ENV NAME World


CMD ["bash"]

# Run app.py when the container launches
#CMD ["sudo", "python", "app/server.py"]

