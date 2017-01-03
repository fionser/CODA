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
* Linux (g++ 4.9+, cmake 3.2+, gnu make 3.8+, perl v5.18.2+, m4 v1.4.6+)
* Mac OSX (llvm 6.1+, cmake 3.2+, gnu make 3.8+, perl v5.18.2+, m4 v1.4.6+)

## Install
	bash build.sh
	cd build/bin

For Linux platforms, we can use multi-threads programming to speed up the evaluation.
`bash build.sh` will ask you to determine the number of threads to use. It writes as
`This platform can use multi-threads. Please set the number of threads:`
and then type into a positive number.

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

## File Format
Different computations (protocols) might need different input formats.
We use the space chararater as the delimination chararater.

### PROT_MEAN
Protocol mean involves numerical attributes. For the reason that the homomorphic encryption
encrypts integers only, we need integer attribute values. Thus, some common magnifier should be
negotiated among all the data contributors in advance. For example, if three-digit precision is acceptable,
then a real number `x` can be converted to an integer value as `ceil(x * 1000)`.

```
#protocol PROT_MEAN
#<number of total attributes>
attr1 attr2 ...
attr1 attr2 ...
attr1 attr2 ...
```

### PROT_CON
```
#protocol PROT_CON
#<number of total attributes> <the size of attr1> <the size of attr2>...
id1 attr1 attr2 ...
id2 attr1 attr2 ...
```
Protocol contingency table involves categorical attributes only. Thus, the categorical attributes are encoded
as nomral values, i.e., `1, 2, 3, ...`. Empty or missing values are encodes as 0.

### PROT_CI2
There are two kinds of inputs in this chi-2 protocol: the genotype data and the phenotype data.

The input format of the genotype is as following.
```
#protocol PROT_CI2 <-- 一行目はプロトコルを示す
#type genotype　　　<-- 二行目はデータ種類を示す
id1 genotype_data1 <-- 三行目からはデータスベース区切りで、id は患者の識別子、genotype_data は 0,1,2 になる値
id2 genotype_data2
```

The input format of the phenotype is as following.

```
#protocol PROT_CI2
#type phenotype
id1 phenotype_data1
id2 phenotype_data2
....
....
```

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

