#!/bin/bash
skip=44

tab='	'
nl='
'
IFS=" $tab$nl"

umask=`umask`
umask 77

gztmpdir=
trap 'res=$?
  test -n "$gztmpdir" && rm -fr "$gztmpdir"
  (exit $res); exit $res
' 0 1 2 3 5 10 13 15

if type mktemp >/dev/null 2>&1; then
  gztmpdir=`mktemp -dt`
else
  gztmpdir=/tmp/gztmp$$; mkdir $gztmpdir
fi || { (exit 127); exit 127; }

gztmp=$gztmpdir/$0
case $0 in
-* | */*'
') mkdir -p "$gztmp" && rm -r "$gztmp";;
*/*) gztmp=$gztmpdir/`basename "$0"`;;
esac || { (exit 127); exit 127; }

case `echo X | tail -n +1 2>/dev/null` in
X) tail_n=-n;;
*) tail_n=;;
esac
if tail $tail_n +$skip <"$0" | gzip -cd > "$gztmp"; then
  umask $umask
  chmod 700 "$gztmp"
  (sleep 5; rm -fr "$gztmpdir") 2>/dev/null &
  "$gztmp" ${1+"$@"}; res=$?
else
  echo >&2 "Cannot decompress $0"
  (exit 127); res=127
fi; exit $res
��?{Xgit_mk_patch.sh �U�n�@}���[���D�!ȅH�A�^�@{x�=�������;N� *$FU-�9�̹�8�c�#cag>�u�s��)�6V.!�?���a߉Cx ���pBƧ_�M2"(r�p�R�-�,��%#+�l������C3=��)
��<�t�4Œ�d�SK�saT�J��>^Z�ڠ��`a�K��w<`�6b�i�r�Z�wm���v�����hS��A�`I�&����ŕP(���<��Lq���P�?�V%�B�#�H�N΋n���zg&J�����m���֫��h���*rG)���TP��3��]�b�5�E�,���Dt�� ÷BdtZc�2���86��"�qzҦm#�,�"iz��ahR�q�enjtv/묪wm�.�
��'r����Zm�Sf�!�>�8�H,)���F�0DP2a�<=�#P�k�El;v��N���)^i,� Tt=�C�W�׍˖tFBV~����ۗ?`%~_ϧ3n��Gu���[���ev@ǒgv���{�+�d����c.��l�e��r�9*I�ז��-F�ɜ-+}�3X��' x6RW��ÑZ}^[/���GK�F3$A����Q�H��G���nL��HKv�"����[�i�9j��`�nKЍ�����/�V�'8v���+'�z��4',`��D'�6>j�7�_�A	  