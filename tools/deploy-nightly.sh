#!/bin/bash
PUBLICATION_BRANCH=master
# set -x
cd $HOME
# Checkout the branch
git clone --branch=$PUBLICATION_BRANCH https://${GITHUB_TOKEN}@github.com/${ARTEFACT_REPO}.git publish
cd publish
# Update pages
BUILD_PATH=$BUILD_DATE-$SHORT_COMMIT_HASH
mkdir $BUILD_PATH
cp $ARTEFACT_BASE/$BUILD_NAME.tar.xz $BUILD_PATH/
cp $ARTEFACT_BASE/MD5SUMS $BUILD_PATH/
cp $ARTEFACT_BASE/SHA256SUMS $BUILD_PATH/
# Write index page
cd $TRAVIS_BUILD_DIR
COMMITS=`git log --oneline | awk '{print $1}'`
cd $HOME/publish
echo "
<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">
<html><head>
	<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">
	<title>$PROJECT_NAME Builds</title>
</head>
<body>
<h2>$PROJECT_NAME Builds</h2>
<table>
" > index.html

for commit in $COMMITS; do
	FILEPATH=`find . -maxdepth 2 -name "*-$commit.tar.xz"`
	if [ "$FILEPATH" != "" ]; then
		FILEDIR=`dirname "${FILEPATH}"`
		FILENAME=`basename "${FILEPATH}"`
		FILEPATH=${FILEPATH:2}
		# pushd "${FILEDIR}" 
		# HASH_MD5=`md5sum --binary ${FILENAME}`
		# HASH_SHA256=`sha256sum --binary ${FILENAME}`
		# popd
		echo "<tr><td><a href=\"$FILEPATH\">$FILENAME</a></td><td><a href=\"$FILEDIR/MD5SUMS\">MD5SUMS</a></td><td><a href=\"$FILEDIR/SHA256SUMS\">SHA256SUMS</a></td></tr>" >> index.html
	fi
	
done

echo "
</table>
</body></html>
" >> index.html

# Commit and push latest version
git add $BUILD_PATH/$BUILD_NAME.tar.xz $BUILD_PATH/MD5SUMS $BUILD_PATH/SHA256SUMS index.html
git config user.name  "Travis"
git config user.email "travis@travis-ci.org"
git commit -m "Build products for $SHORT_COMMIT_HASH, built on $TRAVIS_OS_NAME, log: $TRAVIS_BUILD_WEB_URL"
if [ "$?" != "0" ]; then
	echo "Looks like the commit failed"
fi
git push -fq origin $PUBLICATION_BRANCH