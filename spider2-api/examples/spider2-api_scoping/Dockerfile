FROM alpine:latest
LABEL authors="Jan Hruby <jan@vitrix.cz>"

RUN apk add gcompat

COPY spider2-api_scoping /usr/local/bin/spider2-api_scoping
COPY resources /usr/local/bin/resources

ENTRYPOINT ["spider2-api_scoping"]
