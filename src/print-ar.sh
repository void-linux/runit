cat warn-auto.sh
echo 'main="$1"; shift'
echo 'rm -f "$main"'
echo 'ar cr "$main" ${1+"$@"}'
kern="$(uname -s | tr '/:[A-Z]' '..[a-z])')-$(uname -r | tr /: ..)" || kern="unknown"
case $kern in
  sunos-5.*) ;;
  unix_sv*) ;;
  irix64-*) ;;
  irix-*) ;;
  dgux-*) ;;
  hp-ux-*) ;;
  sco*) ;;
  *) echo 'ranlib "$main"' ;;
esac
