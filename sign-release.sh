#!/bin/bash
cd release
gpg --default-key chiton@edman007.com --clearsign *.dsc
gpg-sign --armor --default-key chiton@edman007.com *
