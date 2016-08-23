FROM ubuntu:latest

RUN apt-get -o Acquire::ForceIPv4=true update && apt-get -o Acquire::ForceIPv4=true install -y libsqlite3-0 python libxml2 python-beautifulsoup ca-certificates

ADD src/prebot fetch.py googl.py /srv/prebot/
ADD plugins/*.so /srv/prebot/plugins/

VOLUME /srv/prebot/var/
ADD var/* /srv/prebot/var/

RUN ln -s /srv/prebot/var/ircbot.cfg /srv/prebot/ircbot.cfg

ENTRYPOINT /srv/prebot/prebot /srv/prebot/var/
