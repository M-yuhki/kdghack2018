#!/bin/sh

chmod 600 ./team3.pem
ssh ec2-user@ec2-18-216-207-93.us-east-2.compute.amazonaws.com -i ./team3.pem
