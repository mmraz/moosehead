FROM ubuntu:16.04

WORKDIR /var/tmp

RUN apt-get update && apt-get -y install curl gcc-5 csh make file gdb && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 1 && \
    curl -O http://hboehm.info/gc/gc_source/gc-7.4.2.tar.gz && \
    curl -O http://hboehm.info/gc/gc_source/libatomic_ops-7.4.0.tar.gz && \
    tar -xzvf gc-7.4.2.tar.gz && rm gc-7.4.2.tar.gz && \
    cd gc-7.4.2 && tar -xzvf ../libatomic_ops-7.4.0.tar.gz && \
    mv libatomic_ops-7.4.0 libatomic_ops && rm ../libatomic_ops-7.4.0.tar.gz && \
    cd libatomic_ops && ./configure --prefix=/usr && make && make install && \
    cd .. && ./configure --prefix=/usr && make && make install && ldconfig

RUN useradd -ms /bin/bash mud

WORKDIR /mud/moosehead/src

COPY src/ .

RUN make

WORKDIR /mud/moosehead

RUN mkdir -p log olc/olcarea gods player jail jerks temp

COPY clan/ clan/
COPY newareas/ newareas/
COPY olc/olcarea/ olc/olcarea/

WORKDIR /mud/moosehead/area

COPY area/ .

RUN chown -R mud /mud/moosehead
USER mud
EXPOSE 4000
CMD ["./startup"]
