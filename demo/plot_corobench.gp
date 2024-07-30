set term qt title "corobench results"
set multiplot layout 2, 1
set xlabel "Number of coroutines"
set ylabel "Time, ns"
plot 'corobench-st.dat' with lines title "with state table" lt rgb "red", \
     'corobench-no-st.dat' with lines title "vanilla libcoro" lt rgb "blue" 

plot 'corobench-no-st.dat' with lines title "vanilla libcoro" lt rgb "blue"
