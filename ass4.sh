./waf --run "scratch/app7 --part=1"
./waf --run "scratch/app7 --part=2"
gnuplot <<- EOF
	set xlabel "time(in s)"
	set ylabel "cwnd(B)"
	set terminal png size 640,480
	set output "a_uniflow_tcpreno_cwnd.png"
	plot "app7_h1_h4_a.cwnd" using 1:2 title "(a) Congestion Window TcpReno"
	set xlabel "time(in s)"
	set ylabel "cwnd(B)"
	set terminal png size 640,480
	set output "a_uniflow_tcpNewReno_cwnd.png"
	plot "app7_h2_h5_a.cwnd" using 1:2 title "(a) Congestion Window TcpNewReno"
	set xlabel "time(in s)"
	set ylabel "cwnd(B)"
	set terminal png size 640,480
	set output "a_uniflow_tcpBic_cwnd.png"
	plot "app7_h3_h6_a.cwnd" using 1:2 title "(a) Congestion Window TcpBic"
	set xlabel "time(in s)"
	set ylabel "Throughput(Kbps)"
	set terminal png size 640,480
	set output "a_uniflow_tcpreno_throughput.png"
	plot "app7_h1_h4_a.tp" using 1:2 title "(a) Throughput TcpReno"
	set xlabel "time(in s)"
	set ylabel "Throughput(Kbps)"
	set terminal png size 640,480
	set output "a_uniflow_tcpNewReno_throughput.png"
	plot "app7_h2_h5_a.tp" using 1:2 title "(a) Throughput TcpNewReno"
	set xlabel "time(in s)"
	set ylabel "Throughput(Kbps)"
	set terminal png size 640,480
	set output "a_uniflow_tcpBic_throughput.png"
	plot "app7_h3_h6_a.tp" using 1:2 title "(a) Throughput TcpBic"
	set xlabel "time(in s)"
	set ylabel "Goodput(Kbps)"
	set terminal png size 640,480
	set output "a_uniflow_tcpreno_goodput.png"
	plot "app7_h1_h4_a.gp" using 1:2 title "(a) Goodput TcpReno"
	set xlabel "time(in s)"
	set ylabel "Goodput(Kbps)"
	set terminal png size 640,480
	set output "a_uniflow_tcpNewReno_goodput.png"
	plot "app7_h2_h5_a.gp" using 1:2 title "(a) Goodput TcpNewReno"
	set xlabel "time(in s)"
	set ylabel "Goodput(Kbps)"
	set terminal png size 640,480
	set output "a_uniflow_tcpBic_goodput.png"
	plot "app7_h3_h6_a.gp" using 1:2 title "(a) Goodput TcpBic"
	
	set xlabel "time(in s)"
	set ylabel "cwnd(B)"
	set terminal png size 640,480
	set output "b_multiflow_tcpreno_cwnd.png"
	plot "app7_h1_h4_b.cwnd" using 1:2 title "(b) Congestion Window TcpReno"
	set xlabel "time(in s)"
	set ylabel "cwnd(B)"
	set terminal png size 640,480
	set output "b_multiflow_tcpNewReno_cwnd.png"
	plot "app7_h2_h5_b.cwnd" using 1:2 title "(b) Congestion Window TcpNewReno"
	set xlabel "time(in s)"
	set ylabel "cwnd(B)"
	set terminal png size 640,480
	set output "b_multiflow_tcpBic_cwnd.png"
	plot "app7_h3_h6_b.cwnd" using 1:2 title "(b) Congestion Window TcpBic"
	set xlabel "time(in s)"
	set ylabel "Throughput(Kbps)"
	set terminal png size 640,480
	set output "b_multiflow_tcpreno_throughput.png"
	plot "app7_h1_h4_b.tp" using 1:2 title "(b) Throughput TcpReno"
	set xlabel "time(in s)"
	set ylabel "Throughput(Kbps)"
	set terminal png size 640,480
	set output "b_multiflow_tcpNewReno_throughput.png"
	plot "app7_h2_h5_b.tp" using 1:2 title "(b) Throughput TcpNewReno"
	set xlabel "time(in s)"
	set ylabel "Throughput(Kbps)"
	set terminal png size 640,480
	set output "b_multiflow_tcpBic_throughput.png"
	plot "app7_h3_h6_b.tp" using 1:2 title "(b) Throughput TcpBic"
	set xlabel "time(in s)"
	set ylabel "Goodput(Kbps)"
	set terminal png size 640,480
	set output "b_multiflow_tcpreno_goodput.png"
	plot "app7_h1_h4_b.gp" using 1:2 title "(b) Goodput TcpReno"
	set xlabel "time(in s)"
	set ylabel "Goodput(Kbps)"
	set terminal png size 640,480
	set output "b_multiflow_tcpNewReno_goodput.png"
	plot "app7_h2_h5_b.gp" using 1:2 title "(b) Goodput TcpNewReno"
	set xlabel "time(in s)"
	set ylabel "Goodput(Kbps)"
	set terminal png size 640,480
	set output "b_multiflow_tcpBic_goodput.png"
	plot "app7_h3_h6_b.gp" using 1:2 title "(b) Goodput TcpBic"
	
EOF
mkdir results
mv *.png results/
cp app7_h1_h4_a.congestion_loss results/a_uniflow_tcpreno_congestion_loss_and_max_tp.txt
cp app7_h2_h5_a.congestion_loss results/a_uniflow_tcpNewReno_congestion_loss_and_max_tp.txt
cp app7_h3_h6_a.congestion_loss results/a_uniflow_tcpBic_congestion_loss_and_max_tp.txt
cp app7_h1_h4_b.congestion_loss results/b_multiflow_tcpreno_congestion_loss_and_max_tp.txt
cp app7_h2_h5_b.congestion_loss results/b_multiflow_tcpNewReno_congestion_loss_and_max_tp.txt
cp app7_h3_h6_b.congestion_loss results/b_multiflow_tcpBic_congestion_loss_and_max_tp.txt
