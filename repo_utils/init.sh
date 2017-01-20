#!/bin/bash

START_DIR="$PWD"

GITBASE_DIR="$(cd ${BASH_SOURCE%/*}/..; pwd)"
if [ ! -e "$GITBASE_DIR/.git" ]
then
  echo "ERROR: Unable to locate .git folder.  Are you sure you're in a repository?"
  exit 1
elif [ ! -d "$GITBASE_DIR/.git" ]
then
  echo "ERROR: .git is a file, not a folder.  Are you sure this isn't a submodule?"
  exit 2
fi

# Ensure all remote branches are up to date
cd "$GITBASE_DIR"
git fetch -p -a

echo "Initializing gerrit code review..."
GERRIT2_DIR="$GITBASE_DIR/.git/gerrit2"
if [ -e "$GERRIT2_DIR" ]
then
  echo "WARNING: $GERRIT2_DIR already exists."
  echo -n "Overwrite existing gerrit configuration? [y/N] "
  read OVERWRITE
  if [[ "${OVERWRITE}" = "y" || "${OVERWRITE}" = "Y" ]]
  then
    rm -rf "$GERRIT2_DIR"
  else
    echo "Aborting gerrit2 initialization"
    exit 4
  fi
fi

mkdir -p "$GERRIT2_DIR"
git clone git://kwsource.kitware.com/computer-vision/git-gerrit.git "$GERRIT2_DIR"
cd "$START_DIR"


# Fix paths on windows
sed -e "s/= \([A-Za-z]\):/= \/\1/" "$GITBASE_DIR/.git/config" > "$GITBASE_DIR/.git/config.new"
mv "$GITBASE_DIR/.git/config.new" "$GITBASE_DIR/.git/config"

GERRIT2_SSH_TEST=0
while [ $GERRIT2_SSH_TEST -ne 1 ]
do
  echo -n "Gerrit hostname: [cvreview.kitware.com] "
  read GERRIT2_HOSTNAME
  if [ -z "${GERRIT2_HOSTNAME}" ]
  then
    GERRIT2_HOSTNAME="cvreview.kitware.com"
  fi
  echo -n "Gerrit ssh port: [29418] "
  read GERRIT2_PORT
  if [ -z "${GERRIT2_PORT}" ]
  then
    GERRIT2_PORT="29418"
  fi
  echo -n "Gerrit username: [$(git config user.email | sed -e 's|@.*||')] "
  read GERRIT2_USER
  if [ -z "${GERRIT2_USER}" ]
  then
    GERRIT2_USER=$(git config user.email | sed -e 's|@.*||')
  fi
  echo -n "Gerrit project: [VidTK] "
  read GERRIT2_PROJECT
  if [ -z "${GERRIT2_PROJECT}" ]
  then
    GERRIT2_PROJECT="VidTK"
  fi
  echo -n "Gerrit target branch: [master] "
  read GERRIT2_TARGET_BRANCH
  if [ -z "${GERRIT2_TARGET_BRANCH}" ]
  then
    GERRIT2_TARGET_BRANCH="master"
  fi

  echo "Testing SSH connection..."
  ssh -p ${GERRIT2_PORT} ${GERRIT2_USER}@${GERRIT2_HOSTNAME}
  if [ $? -eq 127 ]
  then
    GERRIT2_SSH_TEST=1
    echo "SSH connection successful!"
  else
    echo "SSH connection to the Gerrit server failed"
  fi
done

git config gerrit2.hostname ${GERRIT2_HOSTNAME}
git config gerrit2.port ${GERRIT2_PORT}
git config gerrit2.username ${GERRIT2_USER}
git config gerrit2.project ${GERRIT2_PROJECT}
git config gerrit2.targetbranch ${GERRIT2_TARGET_BRANCH}

if [ -n "$(git remote | grep -e '^gerrit2$')" ]
then
  git remote rm gerrit2
fi
git remote add gerrit2 ssh://${GERRIT2_USER}@${GERRIT2_HOSTNAME}:${GERRIT2_PORT}/${GERRIT2_PROJECT}.git

git config alias.gerrit2 "!bash \"$GERRIT2_DIR/git-gerrit2-wrapper.sh\""

echo "Initializing hooks..."
HOOKS_DIR="$GITBASE_DIR/.git/hooks"
if [ -e "$HOOKS_DIR" ]
then
  echo "WARNING: $HOOKS_DIR already exists."
  echo -n "Overwrite existing hooks configuration? [y/N] "
  read OVERWRITE
  if [[ "$OVERWRITE" = "y" || "$OVERWRITE" = "Y" ]]
  then
    rm -rf "$HOOKS_DIR"
  else
    echo "Aborting hooks initialization"
    exit 3
  fi
fi

rm -rf "$GITBASE_DIR/.git/hooks"
mkdir -p "$GITBASE_DIR/.git/hooks"
git clone -b hooks/d git://kwsource.kitware.com/computer-vision/repoutils.git "$GITBASE_DIR/.git/hooks"

rm -rf "$GITBASE_DIR/.git/hooks.vision"
mkdir -p "$GITBASE_DIR/.git/hooks.vision"
git clone -b hooks/vision git://kwsource.kitware.com/computer-vision/repoutils.git "$GITBASE_DIR/.git/hooks.vision"

rm -rf "$GITBASE_DIR/.git/hooks.kitware"
mkdir -p "$GITBASE_DIR/.git/hooks.kitware"
git clone -b hooks/kitware git://kwsource.kitware.com/computer-vision/repoutils.git "$GITBASE_DIR/.git/hooks.kitware"

# Remove old hooks chaining
git config hooks.GerritId false
git config --unset hooks.chain-prepare-commit-msg
git config --unset hooks.chain-commit-msg

# Set up new hooks.d chaining
git config hooks.d "$GITBASE_DIR/.git/hooks.kitware:$GERRIT2_DIR:$GITBASE_DIR/.git/hooks.vision"
