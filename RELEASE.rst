This is just maintainer documentation on how to make a release
1. `./update-external.sh`
2. `cd external/npm/; npm update hls.js; cd -`
3. Update all deps `git commit -a`
4. Update the version in configure.ac
5. Make the release commit `git commit -a`
6. `make maintainer-clean`
7. `./configure`
8. `make distcheck`
9.  `./sign-release.sh`
10. Build the slackware release
    10a. `cd release; mkdir sb; cd sb; tar -xvf ../chiton-*.slackbuild.tar.xz .; cp ../chiton-*.tar.xz chiton`
    10b. `cd chiton; sudo ./chiton.SlackBuild`
    10c. `cp /tmp/chiton-*.txz ../..`
    10d. `cd ..; rm -rf sb`
    10e. `gpg2  --default-key  chiton@edman007.com  -b chiton-*.txz`
    10f. `cd ..`
11. Archive release `mv release/ ../chiton-archive/vX.X.X`
12. Make updates public `git push`
13. Publish!

* Any changes found should be rebased with `git rebase -i` before the actual build
