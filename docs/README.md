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

* make a workding space as following. The `clients` folder indicates the data contributors with plain, unencrypted data files, i.e., `input1.csv` and `input2.csv`. The `server` folder indicates the cloud server which hosts the CODA. The encrypted files will be stored under the folders within
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
* then run *core gen meta/meta.init* to generate the publick key and private key. The `meta` folder then becomes
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
Here `#4` indicates that there are four attributes of the numerical data. For instance, we can have the `age`, `incomes`, `weight` and `height` attributes. All these values should be integer. To handle float reals in CODA, we might need some pre-processing. For example, to save 2-digit precision, we have convert the float value `1.245` to the integer `125`, by multipying `10^2` to the float values.

* Run the command to encrypt Alice's plain data: *core enc clients/Alice/input1.csv server/encrypted_clients/Alice/ meta/meta.init*. The ciphertexts are stored in the folder `server/encrypted_clients/Alice`. Indeed, in a real applicaiton, the ciphertexts are generated on the Alice's side, and then transferred to the server through the network. We in here, omit these kinds of network transferring.

  Also, do the same encryption on Bob's data. *core enc clients/Bob/input2.csv server/encrypted_clients/Bob/ meta/meta.init*. Now the folders on the server's side become 
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

* Run the command to calculate the mean on the ciphertexts: *core eva server/encrypted_clients/ server/evaluation_result/ meta/meta.init -p PROT_MEAN*. This command will do the evaluation from ciphertexts that stored under `sever/encrypted_clients/`. We use the flag `-p PROT_MEAN` to tell CODA to evaluate the mean protocol. The computation result is stored in file `server/evaluation_result/FILE_1`, which is also a ciphertext.

* Run the command to decrypt the ciphertext and get the mean of Alice's and Bob's data: *core dec server/evaluation_result/FILE_1 ./ meta/meta.init*. The fourth argument `./` means that the decrypted file (i.e., `File_result`) will put in the current directory. To this end, the workspace folder becomes
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
        └── FILE_1
```
* We can see the mean from `File_result`: **cat File_result**.
