#!/bin/bash

if [ ! -e generafile ];
then
    echo "Compilare generafile, eseguibile mancante!";
    exit 1
fi
if [ ! -e farm ];
then
    echo "Compilare farm, eseguibile mancante!"
    exit 1
fi

#
# il file expected.txt contiene i risultati attesi per i file
# generati nel seguito con il programma generafile
# expected.txt viene utilizzato per i primi 5 tests
#
cat > expected.txt <<EOF
64834211 file100.dat
103453975 file2.dat
153259244 file1.dat
193031985 testdir/testdir2/file150.dat
258119464 file116.dat
293718900 file3.dat
380867448 file5.dat
518290132 file17.dat
532900090 file117.dat
584164283 file4.dat
748176663 file16.dat
890024017 testdir/file19.dat
985077644 testdir/file8.dat
1146505381 file10.dat
1485251680 file12.dat
1674267050 file13.dat
1878386755 file14.dat
1884778221 testdir/testdir2/file111.dat
2086317503 file15.dat
2322416554 file18.dat
2560452408 file20.dat
EOF

#
# il file expected_usr1.txt contiene, oltre i risultati attesi per i file
# generati nel seguito con il programma generafile, alcuni messaggi di errore
# del programma farm, e il risultato della percezione del segnale SIGUSR1 da parte di farm 
# expected_usr1.txt viene utilizzato per il sesto test
#
cat > expected_usr1.txt <<EOF
ERROR: stat of fakedir1 failed with errno=2 -> stat: No such file or directory
ERROR: stat of fakedir failed with errno=2 -> stat: No such file or directory
64834211 file100.dat
193031985 testdir/testdir2/file150.dat
258119464 file116.dat
890024017 testdir/file19.dat
985077644 testdir/file8.dat
1146505381 file10.dat
1884778221 testdir/testdir2/file111.dat
64834211 file100.dat
103453975 file2.dat
153259244 file1.dat
193031985 testdir/testdir2/file150.dat
258119464 file116.dat
293718900 file3.dat
380867448 file5.dat
518290132 file17.dat
532900090 file117.dat
584164283 file4.dat
748176663 file16.dat
890024017 testdir/file19.dat
985077644 testdir/file8.dat
1146505381 file10.dat
1485251680 file12.dat
1674267050 file13.dat
1878386755 file14.dat
1884778221 testdir/testdir2/file111.dat
2086317503 file15.dat
2322416554 file18.dat
2560452408 file20.dat
EOF

#
# il file expected_timeout.txt contiene alcuni messaggi di errore (uno legato ad un file non esistente,
# e uno legato ad un errore di timeout spiegato meglio nella relazione), e alcuni risultati attesi generati da generafile
# expected_timeout.txt viene utilizzato per il settimo test
#
cat > expected_timeout.txt <<EOF
ERROR: file0.txt is not a regular file or is not a file.dat
64834211 file100.dat
1146505381 file10.dat
EOF

#
# generafile genera i file file100.dat file150.dat file19.dat file116.dat...
# in modo deterministico
#
j=1
for i in 100 150 19 116 2 1 117 3 5 17 4 16 19 8 10 111 12 13 14 15 18 20; do
    ./generafile file$i.dat $(($i*11 + $j*117)) > /dev/null
    j=$(($j+3))
done

mkdir -p testdir
mv file19.dat file8.dat testdir
mkdir -p testdir/testdir2
mv file111.dat file150.dat testdir/testdir2

# esecuzione con 2 thread e coda lunga 1
./farm -n 2 -q 1 file* -d testdir | grep "file*" | awk '{print $1,$2}' | diff - expected.txt
if [[ $? != 0 ]]; then
    echo "test1 failed"
else
    echo "test1 passed"
fi

# esecuzione con 8 thread e coda lunga 16
./farm -d testdir -n 8 -q 16 file* | grep "file*" | awk '{print $1,$2}' | diff - expected.txt
if [[ $? != 0 ]]; then
    echo "test2 failed"
else
    echo "test2 passed"
fi

#
# esecuzione "rallentata" con 1 thread, dopo circa 5 secondi viene
# inviato il segnale SIGTERM (comando pkill) e si valuta l'exit status
# 
./farm -n 1 -d testdir -q 1 -t 1000 file* 2>&1 > /dev/null &
pid=$!
sleep 5
pkill farm
wait $pid
if [[ $? != 0 ]]; then
    echo "test3 failed"
else
    echo "test3 passed"
fi


#
# esecuzione con valgrind. Se valgrind trova dei problemi esce con 
# exit status 1.
#
valgrind --error-exitcode=1 --log-file=/dev/null ./farm -d testdir file* 2>&1 > /dev/null
if [[ $? != 0 ]]; then
    echo "test4 failed"
else
    echo "test4 passed"
fi

# verifica di memory leaks
valgrind --leak-check=full --error-exitcode=1 --log-file=/dev/null ./farm file* -d testdir 2>&1 > /dev/null
if [[ $? != 0 ]]; then
    echo "test5 failed"
else
    echo "test5 passed"
fi

#
# esecuzione "rallentata" con 2 threads, dopo circa 2 secondi viene
# inviato il segnale SIGUSR1 (tranute comando kill) e si confronta il risultato con diff - expected_usr1.txt
# ci aspettiamo di ottenere, prima della stampa di tutti i risultati, la stampa dei risultati ottenuti nei primi 2 secondi
# 
./farm -n 2 -q 6 -d testdir -d fakedir -d fakedir1 -t 300 file* 2>&1 | grep "file*" | diff - expected_usr1.txt &
#echo $pid && 
#pidof -s farm
pid=$(pidof -s farm)
sleep 2
kill -USR1 $(pidof farm)
while [ -e /proc/$pid ]
do
    sleep .6
done
if [[ $? != 0 ]]; then
    echo "test6 failed"
else
    echo "test6 passed"
fi

#
# esecuzione con coda lunga 2, dove brokenfarm chiude i workers tramite return NULL; dopo il loro primo task (spiegazione completa nella relazione).
# oltre al messaggio di errore dovuto a file0.txt (inserito appositamente per il test), ci aspettiamo il risultato dei primi 2 task passati alla
# coda ed eseguiti poi dai Workers. Master esce accorgendosi che non sono presenti più Workers in esecuzione
# 
# lancio test 7 se presente eseguibile brokenfarm
if [ ! -e brokenfarm ]; 
then
    echo "test7 not launched (check project report)";
else
    touch file0.txt
    ./brokenfarm -n 2 -q 2 file* 2>&1 | grep "file*" | diff - expected_timeout.txt
    if [[ $? != 0 ]]; then
        echo "test7 failed"
    else
        echo "test7 passed"
    fi
    rm file0.txt
fi