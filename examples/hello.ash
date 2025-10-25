# Esempio script .ash
# Dimostrazione sequenze, pipeline, variabili.

echo "Inizio script"
export NAME=Luigi
echo "Ciao $NAME"
ls | grep cpp || echo "Nessun cpp"
# Background job
echo avvio sleep &
sleep 1 &
jobs
fg 1
echo "Fine script (status precedente=$?)"
