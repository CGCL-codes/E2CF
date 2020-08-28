# E2CF: The Entry-Extensible Cuckoo Filter
The entry-extensible cuckoo filter (E2CF) supports entry-level extension for dynamic set representation and accelerates the membership query. E2CF utilizes adjacent buckets with sequential physical addresses in a cuckoo filter to extend bucket entries, which avoids many discrete memory accesses in a query. To further make E2CF space and time efficient, we adopt asynchronous extension and fine-grained splitting methods. Experiment results show that compared to existing designs, E2CF reduces the query and insertion time by up to 82% and 28%, respectively.

## How to use?
### Environment
We implement E2CF in a Red Hat Enterprise Linux Server release 6.2 with an Intel 2.60GHz CPU and 64GB RAM, the size of a cache line in the server is 64 bytes, and we configure OpenSSL library environment in the server. 

Install OpenSSL (Please refer to https://www.openssl.org to learn more).

```txt
sudo apt-get install openssl
sudo apt-get install libssl-dev
```
Build and run the example

```txt
make
./test
```


### Configurations
Configurations including false pisitive, item number and dataset path can be costomized in "configuration/config.txt". 

```txt
false positive = 0.02
item number = 100
input file path = input/input100.txt
```


## Author and Copyright

E2CF is developed in Big Data Technology and System Lab, Services Computing Technology and System Lab, School of Computer Science and Technology, Huazhong University of Science and Technology, Wuhan, China by Shuiying Yu (shuiying@hust.edu.cn), Sijie Wu (wsj@hust.edu.cn), Hanhua Chen (chen@hust.edu.cn), Hai Jin (hjin@hust.edu.cn)

Copyright (C) 2020, [STCS & CGCL](http://grid.hust.edu.cn/) and [Huazhong University of Science and Technology](http://www.hust.edu.cn).

## Publication

If you want to know more detailed information, please refer to this paper:

Shuiying Yu, Sijie Wu, Hanhua Chen, Hai Jin, "The Entry-Extensible Cuckoo Filter," Proceedings of The 17th Annual IFIP International Conference on Network and Parallel Computing (NPC 2020), Zhengzhou, Henan, China , Sept.28-30, 2020.
