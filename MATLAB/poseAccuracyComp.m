function [pose_err,roll_err,pitch_err,yaw_err] = poseAccuracyComp(dvsData,optitrackData,timeOffsetOpti,outputPrefixName)

Pd_raw = importdata(dvsData);
Po = importdata(optitrackData);

%% Data extraction
%eliminate transformation duplicates in dvs data (ignore timestamp)
[Pd,ia,ic] = unique(Pd_raw(:,1:6),'rows','stable');
Pd = [Pd Pd_raw(ia,15)];

%dvs data
Td = Pd(:,1:3);
V_r = Pd(:,4:6);

timeD = Pd(:,7);
timeD = timeD - timeD(1);

RPY = zeros(size(V_r));

[m n] = size(V_r);

for i = 1:m
    RPY(i,:) = rpyAng(rodrigues(V_r(i,:)));
end;

RPY = radtodeg(RPY);

rollD = RPY(:,1);
pitchD = RPY(:,2);
yawD = RPY(:,3);

%optitrack data
To = Po(:,[2 1 3]);
To(:,2) = -To(:,2);

Qo = Po(:,[7 4 5 6]);   %format of quat input is [x y z w]

timeO = Po(:,8);
timeO = timeO - timeO(1) + timeOffsetOpti;

% eliminate dvs data which is not covered by optitrack
startO = timeO(1);
endO = timeO(end);

i_interval = timeD > startO & timeD < endO;

Td = Td(i_interval,:);
timeD = timeD(i_interval);

rollD = rollD(i_interval);
pitchD = pitchD(i_interval);
yawD = yawD(i_interval);

% interpolate optitrack translation data
To_i(:,1) = interp1(timeO,To(:,1),timeD);
To_i(:,2) = interp1(timeO,To(:,2),timeD);
To_i(:,3) = interp1(timeO,To(:,3),timeD);

%% Transformation to DVS reference frame
%determine rotation and translation to align both ref frames

Q0 = ones(4,1);
t0 = zeros(3,1);

x0 = [Q0 ; t0];

x = findTransformation(x0,Td,To_i);

Q = x(1:4,1)';
t = x(5:7,1);

% align optitrack to dvs referece frame
[m n] = size(To_i);
To_i = quatrotate(Q,To_i) + repmat(t',m,1); %interpolated data

[m n] = size(To);
To_r = quatrotate(Q,To) + repmat(t',m,1);   %raw optitrack data

Qo_aligned = quatmultiply(Q,Qo);

[yawO,pitchO,rollO] = quat2angle(Qo_aligned);

rollO = radtodeg(rollO);
pitchO = radtodeg(pitchO);
yawO = radtodeg(yawO);

%determine translation error
distance_V = (Td - To_i)';
pose_err = (sqrt(sum(distance_V.^2)))';

%interpolate optitrack rotation data
rollO_i = interp1(timeO,rollO,timeD);
pitchO_i = interp1(timeO,pitchO,timeD);
yawO_i = interp1(timeO,yawO,timeD);

% Align axis rotations
x0 = 0;
shift = findRotationShift(x0,rollD,rollO_i);
rollO_i = rollO_i + shift;
rollO = rollO + shift;

shift = findRotationShift(x0,pitchD,pitchO_i);
pitchO_i = pitchO_i + shift;
pitchO = pitchO + shift;

shift = findRotationShift(x0,yawD,yawO_i);
yawO_i = yawO_i + shift;
yawO = yawO + shift;

%determine rotation error
yaw_err = (abs(yawO_i - yawD));
pitch_err = (abs(pitchO_i - pitchD));
roll_err = (abs(rollO_i - rollD));

%% Plotting
% translation plot
%figure;

style = '-';

subplot(2,2,1); plot(timeD,Td(:,1),style,timeO,To_r(:,1),style);
title('Translation x-axis','FontSize',12,'FontWeight','bold');
xlabel('Time [s]');
ylabel('Distance [m]');
legend('DVS','OptiTrack');

subplot(2,2,2); plot(timeD,Td(:,2),style,timeO,To_r(:,2),style);
title('Translation y-axis','FontSize',12,'FontWeight','bold');
xlabel('Time [s]');
ylabel('Distance [m]');
legend('DVS','OptiTrack');

subplot(2,2,3); plot(timeD,Td(:,3),style,timeO,To_r(:,3),style);
title('Translation z-axis','FontSize',12,'FontWeight','bold');
xlabel('Time [s]');
ylabel('Distance [m]');
legend('DVS','OptiTrack');

h = gcf;
saveas(h,[outputPrefixName '_translation'],'fig');

% rotation plot
%figure;

subplot(2,2,1); plot(timeD,yawD,style,timeO,yawO,style);
title('Yaw','FontSize',12,'FontWeight','bold');
xlabel('Time [s]');
ylabel('Degree');
ylim([-200 200]);
legend('DVS','OptiTrack');

subplot(2,2,2); plot(timeD,pitchD,style,timeO,pitchO,style);
title('Pitch','FontSize',12,'FontWeight','bold');
xlabel('Time [s]');
ylabel('Degree');
ylim([-200 200]);
legend('DVS','OptiTrack');

subplot(2,2,3); plot(timeD,rollD,style,timeO,rollO,style);
title('Roll','FontSize',12,'FontWeight','bold');
xlabel('Time [s]');
ylabel('Degree');
ylim([-200 200]);
legend('DVS','OptiTrack');

h = gcf;
saveas(h,[outputPrefixName '_rotation'],'fig');