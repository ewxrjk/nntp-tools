FROM debian:12
RUN apt-get update -y && \
	apt-get install -y --no-install-recommends \
    autoconf \
    automake \
	build-essential \
    inn2-dev \
    libcairomm-1.0-dev \
    libcurl4-gnutls-dev \
    libexpat1-dev \
    libgcrypt20-dev \
	&& \
	apt-get clean
ADD build /build
VOLUME /src
WORKDIR /src
CMD /build
