FROM centos:7

RUN yum update
RUN yum install -y gcc
RUN yum install -y make nasm
RUN yum install -y mc file

CMD sleep 36000

WORKDIR /cybos/source