from ubuntu:bionic
RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y wget sudo vim nano

ADD files/* /tmp/
RUN /tmp/adduser.sh

RUN adduser `cat /tmp/user` sudo
RUN wget https://raw.githubusercontent.com/sq5bpf/telive/master/scripts/install_telive.sh
RUN chmod 755 install_telive.sh
RUN echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/nopwd
RUN su `cat /tmp/user` ./install_telive.sh
