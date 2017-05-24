Crytographically Secure Statistical Data Analysis (CODA)
==
CODA is built for doing secure outsourcing, specifically for the computation of statistics.
The statistics supported by CODA:

* Mean/Variance/Covariance
* Histogram
* Contingency table (cell-suppression supported)
* Percentile
* Principle component analysis
* Linear regression

This is project is based on this [paper](https://www.internetsociety.org/doc/using-fully-homomorphic-encryption-statistical-analysis-categorical-ordinal-and-numerical-data).

## Running Examples
We give some running examples to show how CODA works.

### Mean Protocol
We compute the mean (average) from numerical data.

* make a working space as following. The `clients` folder indicates the data contributors with plain, unencrypted data files, i.e., `input1.csv` and `input2.csv`. The `server` folder indicates the cloud server which hosts the CODA. The encrypted files will be stored under the folders within
`encrypted_clients`.

```
workspace/
├── clients
│   ├── Alice
│   │   └── input1.csv
│   └── Bob
│       └── input2.csv
├── meta
│   └── meta.init
└── server
    ├── encrypted_clients
    │   ├── Alice
    │   └── Bob
    └── evaluation_result
```

* add this line `protocol PROT_MEAN` to the `meta.init` file.
* then run _core gen meta/meta.init_ to generate the public key and private key. The `meta` folder then becomes
```
workspace/meta/
├── fhe_key.ctxt
├── fhe_key.pk
├── fhe_key.sk
└── meta.init
```
* The input files (i.e., input1.csv and input2.csv) of the clients are in a form as 

```
#4
1 20 3 4
3 2 3 6
2 28 6 4
3 5 92 4
1 2 32 41

```
Here `#4` indicates that there are four attributes of the numerical data. For instance, we can have the `age`, `incomes`, `weight` and `height` attributes. All these values should be integer. To handle float reals in CODA, we might need some pre-processing. For example, to save 2-digit precision, we have convert the float value `1.245` to the integer `125`, by multiplying `10^2` to the float values.

* Run the command to encrypt Alice's plain data: _core enc clients/Alice/input1.csv server/encrypted_clients/Alice/ meta/meta.init_. The ciphertexts are stored in the folder `server/encrypted_clients/Alice`. Indeed, in a real application, the ciphertexts are generated on the Alice's side, and then transferred to the server through the network. We in here, omit these kinds of network transferring.

  A_so, do the same encryption on Bob's data. _core enc clients/Bob/input2.csv server/encrypted_clients/Bob/ meta/meta.init_. Now the folders on the server's side become 
  ```
  server/
  ├── encrypted_clients
  │   ├── Alice
  │   │   └── FILE_1
  │   └── Bob
  │       └── FILE_1
  └── evaluation_result
  ```
  The `FILE_1` are the encryption of Alice's and Bob's plain data.

* Run the command to calculate the mean on the ciphertexts: _core eva server/encrypted_clients/ server/evaluation_result/ meta/meta.init_. This command will do the evaluation from ciphertexts that stored under `sever/encrypted_clients/`. We use the flag `-p PROT_MEAN` to tell CODA to evaluate the mean protocol. The computation result is stored in file `server/evaluation_result/FILE_result`, which is also a ciphertext.

* Run the command to decrypt the ciphertext and get the mean of Alice's and Bob's data: _core dec server/evaluation_result/FILE_result ./ meta/meta.init_. The fourth argument `./` means that the decrypted file (i.e., `File_result` using same name but decrypted file) will put in the current directory. To this end, the workspace folder becomes
```
workspace
├── File_result
├── clients
│   ├── Alice
│   │   └── input1.csv
│   └── Bob
│       └── input2.csv
├── meta
│   ├── fhe_key.ctxt
│   ├── fhe_key.pk
│   ├── fhe_key.sk
│   └── meta.init
└── server
    ├── encrypted_clients
    │   ├── Alice
    │   │   └── FILE_1
    │   └── Bob
    │       └── FILE_1
    └── evaluation_result
        └── File_result
```
* We can see the mean from `File_result`: _cat File_result_.

### Contingency Table Protocol
Contingency table is calculated from categorical data. In this example, the Alice's  and Bob's data is 
```
#2 2 2 
1 1
1 1
1 2
1 1
1 1
2 2
2 2
```
The first line `#2 2 2` is `#number_of_categorical_attributes domain_size_of_the_first_attribute domain_size_of_the_second_attribute`.
For example, if we have two categorical attributes like, "gender (i.e., male and female)" and "health condition (i.e., healthy and ill)",
then the first line of the data file should be `#2 2 2`. If we have another attribute like "age (i.e, <10, 10-20, 30-40, >40)",
then we need to set the first line as `#3 2 2 4`.
For the record `1 2`, it indicates that if might be a record of "male" and "ill". Of course, we can designate `1` as female and `2` as male.

Given Alice's and Bob's data (here, we use the same file for Alice and Bob just for the demonstration) the contingency table is
```
   1   2
   -----
1| 8   2
2| 4   0
```
This means, in total, we have 8 records of `(1, 1)`, 2 records of `(1, 2)` and 2 records of `(2, 1)` but no records of `(2, 2)`.

In this example, we need to set the `meta.init` as `protocol PROT_CON` (CON denotes contingency).
Then the encryption with CODA is _core enc clients/Alice/input1.csv meta/meta.init server/encrypted_clients/Alice_.

The evaluation of the contingency table is a little bit different from the mean protocol.
We can have more than 2 categorical attributes in our data.
Thereby, we need to determine which attribute-pairs to compute when we call the `core eva` command.
In this, example, we call __core eva server/encrypted_clients/ server/evaluation_result/ meta/meta.init 1 2 2_. 
We have three more arguments here, `1 2 2`. The first `1 2` means that we are computing the contingency table with the 1st
and 2nd categorical attributes of the data. The last `2` indicates that we want to hide the values that is not larger than 2 
in the contingency table. For this threshold `2`, the computed contingency is as following.

```
   1   2
   -----
1| 8   0
2| 4   0
```
Since there only two records of `(1 2)`, then this values will be hidden. We note that, the processing is done inside the ciphertexts.
Thus, CODA does not aware of the actual numbers of `(1 2)`.

After we decrypt the evaluation_result as _core dec server/evaluation_result/File_result . meta/meta.init_. 
The computed contingency table is 

```
1 2
8 0 0 4
```
The first line `1 2` indicates that this evaluation is computed from the 1st attribute and the 2nd attribute.

