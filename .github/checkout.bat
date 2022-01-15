git rev-parse --abbrev-ref HEAD > branch.txt

set /p desktop_branch=<branch.txt

echo branch is %desktop_branch%

del branch.txt

cd cryptonote

git checkout %desktop_branch%

git show-ref --verify --quiet refs/heads/%desktop_branch% || git checkout development
