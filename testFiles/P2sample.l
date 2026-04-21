.  SOURCE CODE FOR THE XE VERSION OF THE SIC FAMILY OF COMPUTER
0000    SUM      START    0                        
0000    FIRST    LDX     #0                        050000
0003             LDA     #0                        010000
0006            +LDB     #TABLE2                   69101794
000A             BASE     TABLE2                   
000A    LOOP     ADD      TABLE,X                  1BA017
000D             ADD      TABLE2,X                 1BC000
0010             TIX      COUNT                    2F200E
0013             JLT      LOOP                     3B2FF3
0016            +STA      TOTAL                    0F102F04
001A             RSUB                              4F0000
001D    MYLIT   +LDA     =C'EOF'                   03102F07
0021    COUNT    RESW     1                        
0024    TABLE    RESW     2000                     
1794    TABLE2   RESW     2000                     
2F04    TOTAL    RESW     1                        
2F07    *       =C'EOF'                            454F46
                 END      FIRST                    