CODA
====
---

## Overview  
Cryptographically secure Of statistical Data Analaysis.

## Description
* core - core is an engine to encrypt decrypt eval, written in c++.
* ui/client - client is an manager to comunicate, resources like filesystem on the client computer.  written in c++.
* ui/server - server is an manager to comunicate, resources like filesystem, on the server. written in c++.

## Requirement
* Linux (g++ 3.6+, cmake 3.2+, perl v5.18.2+, m4 v1.4.6+)
* Mac OSX (g++ 3.6+, cmake 3.2+, perl v5.18.2+, m4 v1.4.6+)

## Install
	cd build
	./build.script
	./build.script -DBUILD_lib=on
	./build.script -DBUILD_core=on
	./build.script -DBUILD_ui=on
	cd bin

## Licence
CODA is distributed under the terms of the [GNU General Public License ](https://www.gnu.org/licenses/gpl.html) (GPL).  


## Usage
### core  
	core gen <meta file path>
	core enc [-l type] <input file path> <output dir path> <meta file path>
	core dec <input file path> <output dir path> <meta file path>
	core eva <session dir path> <output dir path> <meta file path> -p <protocol> [<data> ...]
### client  
	client init <hostname> <portno> <session_name> <analyst_name> <protocol> <schema_file_path> <user_name ...>
	client send_key <hostname> <portno> <session_name> <analyst_name>
	client join <hostname> <portno> <session_name> <analyst_name> <user_name>
	client send_data <hostname> <portno> <session_name> <analyst_name> <user_name>
	client receive_result <hostname> <portno> <session_name> <analyst_name>
	client conv [-e] <session_name> <csv_data_file_path>
	client conv [-d] <session_name> <result_file_path>
### server  
	server start <port_no>


## Example
This sample data is the following.
  
	HOST NAME : localhost  
	PORT NO : 5000  
	SESSION NAME : session_name  
	ANALYST NAME : analyst  
	USER NAME : user_a user_b  
	PROTOCOL : PROT_CON

You can execute sample by the following command.
	
### cloud
	server start 5000
### analyst
	client init localhost 5000 session_name analyst PROT_CON schema.csv user_a user_b
	core gen session_name/Meta/meta.ini
	client send_key localhost 5000 session_name analyst
	
### user A (data contributor)
	client join localhost 5000 session_name analyst user_a
	client conv -e session_name data_user_a.csv
	core enc converted_file.txt session_name/Data/Enc/Send session_name/Meta/meta.ini --local PROT_CON
	client send_data localhost 5000 session_name analyst user_a
	
### user B (data contributor)
	client join localhost 5000 session_name analyst user_b
	client conv -e session_name data_user_b.csv
	core enc converted_file.txt session_name/Data/Enc/Send session_name/Meta/meta.ini --local PROT_CON
	client send_data localhost 5000 session_name analyst user_b
	
### cloud
	core eva analyst/session_name analyst/session_name/result analyst/session_name/Meta/meta.ini -p PROT_CON 2 5
	
### analyst
	client receive_result localhost 5000 session_name analyst -p con 2 5
	core dec session_name/Data/Enc/Receive/File_result session_name/Data/Plain/result session_name/Meta/meta.ini
	cat session_name/Data/Plain/result/File_1
	
