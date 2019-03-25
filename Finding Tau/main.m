clc;
clear all;
close all;

%test data
t_test =  [4.84
5.224
5.607
5.989
6.41
6.752
6.948
7.087
7.215
7.343
7.485
7.613
7.741
7.870
7.999
8.127];

vel_test = [26.0
26.0
26.1
26.2
26.1
29.2
51.1
72.1
77.8
78.1
77.5
78.5
78.2
77.4
77.6
78.0];


t_test = transpose(t_test);
vel_test = transpose(vel_test);
plot(t_test,vel_test);
hold on;



% values from test to be used in simulation 
% a of 51 cm/s^s is applied at t = 6.5s for an ???
% acceleration takes place t = 6.5 and t = 7.2
totalTime = 15; %seconds
vo = 26; %cm/s
a_mag = 52; %cm/s^2

dt = .000001; %time step in sim
t = 0:dt:totalTime;
u = zeros(length(t),1);
for j = 1:length(t)
    if (t(j) > 6.3 && t(j) < 7.3)
            u(j) = a_mag;
    end
end


T = .01;
A = [-1/T 0; 1 0];
B = [1/T; 0];
C = [0 1]; 
D = [0];
xo = [0;vo];
sys =ss(A,B,C,D);
[y,to,x] = lsim(sys,u,t,xo);
plot(to,y);


T = .1;
A = [-1/T 0; 1 0];
B = [1/T; 0];
C = [0 1]; 
D = [0];
xo = [0;vo];
sys =ss(A,B,C,D);
[y,to,x] = lsim(sys,u,t,xo);
plot(to,y);


T = 1;
A = [-1/T 0; 1 0];
B = [1/T; 0];
C = [0 1]; 
D = [0];
xo = [0;vo];
sys =ss(A,B,C,D);
[y,to,x] = lsim(sys,u,t,xo);
plot(to,y);


legend('Experimental','Tau = .01','Tau = .1','Tau = 1');
