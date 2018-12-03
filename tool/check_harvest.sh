#!/bin/sh

URL=xxx
KEY=yyy
TOKEN=zzz

curl -X GET -H "X-Soracom-API-Key: ${KEY}" -H "X-Soracom-Token: ${TOKEN}" ${URL}