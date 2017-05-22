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

*PREPARTION.* 

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
* then run `core gen meta/meta.init` to generate the publick key and private key. The `meta` folder then becomes
```
workspace/meta/
├── fhe_key.ctxt
├── fhe_key.pk
├── fhe_key.sk
└── meta.init
```
* The input files of the clients (i.e., Alice and Bob) are in a form as 

```
#4
1 20 3 4
3 2 3 6
2 28 6 4
3 5 92 4
1 2 32 41

```
Here `#4` indicates that there are four attributes of the numerical data. For instance, we can have the `age`, `incomes`, `weight` and `height`.
All these values should be integer. To handle float reals in CODA, we might need some pre-processing. For example, to save 2-digit precision,
we have convert the float value `1.245` to the integer `125`, by multipying `10^2` to the float values.
