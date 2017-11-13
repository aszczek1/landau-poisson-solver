//advection and moments routines

double Gridv(double m){ //v in [-Lv,Lv]
	return (-Lv+(m+0.5)*dv);
}

double Gridx(double m){ // x in [0,Lx]  (returns the x value at the mth discrete space-step
	return (m+0.5)*dx;
}

double rho_x(double x, double *U, int i) // for x in I_i
{
  int j, k;
  double tmp=0.;
  //#pragma omp parallel for shared(U) reduction(+:tmp)
  for(j=0;j<size_v;j++){
	k=i*size_v + j;
	tmp += U[k*6+0] + U[k*6+1]*(x-Gridx((double)i))/dx + U[k*6+5]/4.;
  }
  
  return dv*dv*dv*tmp;
}

double rho(double *U, int i) //  \int f(x,v) dxdv in I_i * K_j
{
  int j, k;
  double tmp=0.;
 // #pragma omp parallel for shared(U) reduction(+:tmp)
  for(j=0;j<size_v;j++){
	k=i*size_v + j;
	tmp += U[k*6+0] + U[k*6+5]/4.;
  }
  
  return dx*dv*dv*dv*tmp;
}


double computePhi_x_0(double *U) // compute the constant coefficient of x in phi, which is actually phi_x(0) (Calculate C_E in the paper -between eq. 52 & 53?)
{
	int i, j, k, m, q;
	double tmp=0.;

	//#pragma omp parallel for private(j,q,m,k) shared(U) reduction(+:tmp) //reduction may change the final result a little bit
	for(j=0;j<size_v;j++){
		//j = j1*Nv*Nv + j2*Nv + j3;
		for(q=0;q<Nx;q++){
			for(m=0;m<q;m++){			
				k=m*size_v + j;
				tmp += U[k*6] + U[k*6+5]/4.;
			}
			k=q*size_v + j;
			tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
		}
	}
	tmp = tmp*scalev*dx*dx;

	return 0.5*Lx - tmp/Lx;
}

double computeC_rho(double *U, int i) // sum_m=0..i-1 int_{I_m} rho(z)dz   (Calculate integral of rho_h(z,t) from 0 to x as in eq. 53)
{
	double retn=0.;
	int j, k, m;
	for (m=0;m<i;m++){ //BUG: was "m < i-1"
		for(j=0;j<size_v;j++){
			k = m*size_v + j;
			retn += U[k*6+0] + U[k*6+5]/4.;
		}
	}
	
	retn *= dx*scalev;
	return retn;
}

double Int_Int_rho(double *U, int i) // \int_{I_i} [ \int^{x}_{x_i-0.5} rho(z)dz ] dx
{
  int j, k;
  double retn=0.;
  for(j=0;j<size_v;j++){
    k=i*size_v + j;
    retn += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
  }
  
  return retn*dx*dx*scalev;  
}

double Int_Int_rho1st(double *U, int i)// \int_{I_i} [(x-x_i)/delta_x * \int^{x}_{x_i-0.5} rho(z)dz ] dx
{
 int j, k;
 double retn=0.;
 for(j=0;j<size_v;j++){
    k=i*size_v + j;
    retn += (U[k*6+0] + U[k*6+5]/4.)/12.;
  }
  return retn*dx*dx*scalev; 
}

/*double Int_Cumulativerho(double **U, int i)// \int_{I_i} [ \int^{x}_{0} rho(z)dz ] dx
{
  double retn=0., cp, tmp;
  cp = computeC_rho(U,i);
  tmp = Int_Int_rho(U,i);
  
  retn = dx*cp + tmp;
  return retn;  
}

double Int_Cumulativerho_sqr(double **U, int i)// \int_{I_i} [ \int^{x}_{0} rho(z)dz ]^2 dx
{
  double retn=0., cp, tmp1, tmp2, tmp3, c1=0., c2=0.;
  int j, k;
  cp = computeC_rho(U,i);
  tmp1 = cp*cp*dx;
  tmp2 = 2*cp*Int_Int_rho(U,i);
  for(j=0;j<size_v;j++){
    k=i*size_v + j;
    c1 += U[k][0] + U[k][5]/4.;
    c2 += U[k][1];
  }
  c2 *= dx/2.;
  tmp3 = pow(dv, 6)* ( c1*c1*dx*dx*dx/3. + c2*c2*dx/30. + c1*c2*(dx*Gridx((double)i)/6. - dx*dx/4.) );
  retn = tmp1 + tmp2 + tmp3;
  return retn;  
}*/

double Int_E(double *U, int i) // \int_i E dx      // Function to calculate the integral of E_h w.r.t. x over the interval I_i = [x_(i-1/2), x_(i+1/2))
{
	int m, j, k;
	double tmp=0., result;
	//#pragma omp parallel for shared(U) reduction(+:tmp)
	for(j=0;j<size_v;j++){
		for(m=0;m<i;m++){	
			k=m*size_v + j;
			tmp += U[k*6+0] + U[k*6+5]/4.;
		}
		k=i*size_v + j;
		tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;		
	}

	//ce = computePhi_x_0(U);
	result = -ce*dx - tmp*dx*dx*scalev + Gridx((double)i)*dx;

	return result;
}

double Int_E1st(double *U, int i) // \int_i E*(x-x_i)/delta_x dx
{
	int j, k;
	double tmp=0., result;
	//#pragma omp parallel for reduction(+:tmp)
	for(j=0;j<size_v;j++){
		k=i*size_v + j;
		tmp += U[k*6+0] + U[k*6+5]/4.;
	}
	tmp = tmp*scalev;
	
	result = (1-tmp)*dx*dx/12.;

	return result;
}

double Int_fE(double *U, int i, int j) // \int f * E(f) dxdv on element I_i * K_j
{
	double retn=0.;
	int k;
	k = i*size_v + j;

	//retn = (U[k][0] + U[k][5]/4.)*Int_E(U,i) + U[k][1]*Int_E1st(U,i);
	retn = (U[k*6+0] + U[k*6+5]/4.)*intE[i] + U[k*6+1]*intE1[i];
	return retn*scalev;
}



double Int_E2nd(double *U, int i) // \int_i E* [(x-x_i)/delta_x]^2 dx
{
    int m, j, j1, j2, j3, k;
    double c1=0., c2=0., result;
  
    //cp = computeC_rho(U,i); ce = computePhi_x_0(U);

    for(j=0;j<size_v;j++){
	    k=i*size_v + j;
	    c1 += U[k*6+0] + U[k*6+5]/4.;
	    c2 += U[k*6+1];
    }
    c2 *= dx/2.;				
    
    result = (-cp[i] - ce+scalev*(c1*Gridx(i-0.5) + 0.25*c2))*dx/12. + (1-scalev*c1)*dx*Gridx((double)i)/12. - scalev*c2*dx/80.; //BUG: missed -cp

    return result;
}

double computeMass(double *U)
{
  int k;
  double tmp=0.;
  #pragma omp parallel for shared(U) reduction(+:tmp)
  for(k=0;k<Nx*size_v;k++) tmp += U[k*6+0] + U[k*6+5]/4.;
  
  return tmp*dx*scalev;
}

void computeMomentum(double *U, double *a)
{
  int k, i, j,j1,j2,j3; //k=i*Nv*Nv*Nv + (j1*Nv*Nv + j2*Nv + j3);	
  double tmp1=0., tmp2=0., tmp3=0.; 
  a[0]=0.; a[1]=0.; a[2]=0.; // the three momentum
  #pragma omp parallel for shared(U) reduction(+:tmp1, tmp2, tmp3)  //reduction directive may change the result a little bit
  for(k=0;k<Nx*size_v;k++){
    j=k%size_v; i=(k-j)/size_v;
    j3=j%Nv; j2=((j-j3)%(Nv*Nv))/Nv; j1=(j-j3-j2*Nv)/(Nv*Nv);
    tmp1 += Gridv((double)j1)*dv*U[k*6+0] + U[k*6+2]*dv*dv/12. + U[k*6+5]*Gridv((double)j1)*dv/4.;
    tmp2 += Gridv((double)j2)*dv*U[k*6+0] + U[k*6+3]*dv*dv/12. + U[k*6+5]*Gridv((double)j2)*dv/4.;
    tmp3 += Gridv((double)j3)*dv*U[k*6+0] + U[k*6+4]*dv*dv/12. + U[k*6+5]*Gridv((double)j3)*dv/4.;
  }
  a[0]=tmp1*dx*dv*dv; a[1]=tmp2*dx*dv*dv; a[2]=tmp3*dx*dv*dv; 
}

double computeKiE(double *U)
{
  int k, i, j,j1,j2,j3; 
  double tmp=0., tp=0., tp1=0.;  
  //#pragma omp parallel for private(k,i,j,j1,j2,j3,tp, tp1) shared(U) reduction(+:tmp)
  for(k=0;k<Nx*size_v;k++){
    j=k%size_v; i=(k-j)/size_v;
    j3=j%Nv; j2=((j-j3)%(Nv*Nv))/Nv; j1=(j-j3-j2*Nv)/(Nv*Nv);
    //tp = ( pow(Gridv(j1+0.5), 3)- pow(Gridv(j1-0.5), 3) + pow(Gridv(j2+0.5), 3)- pow(Gridv(j2-0.5), 3) + pow(Gridv(j3+0.5), 3)- pow(Gridv(j3-0.5), 3) )/3.;
    tp1 = Gridv((double)j1)*Gridv((double)j1) + Gridv((double)j2)*Gridv((double)j2) + Gridv((double)j3)*Gridv((double)j3);
    //tmp += U[k][0]*tp + (Gridv(j1)*U[k][2]+Gridv(j2)*U[k][3]+Gridv(j3)*U[k][4])*dv*dv/6. + U[k][5]*( dv*(dv*dv*3./80. + tp1/12.) + tp/6. );	 
    tmp += U[k*6+0]*(tp1 + dv*dv/4.)*dv + (Gridv(j1)*U[k*6+2]+Gridv(j2)*U[k*6+3]+Gridv(j3)*U[k*6+4])*dv*dv/6. + U[k*6+5]*( dv*dv*dv*19./240. + tp1*dv/4.);	 
  }
  tmp *= dx*dv*dv; 
  return 0.5*tmp;
}

double computeEleE(double *U)
{
  int k, i, j;
  double retn, tmp1=0., tmp2=0., tmp3=0., tmp4=0., tmp5=0., tmp6=0., tmp7=0., tp1, tp2, c;
  double ce1, cp1;
  ce1 = computePhi_x_0(U);
  
  tmp1 = ce1*ce1*Lx;
  tmp2 = Lx*Lx*Lx/3.; tmp3 = -ce1*Lx*Lx;
  
  //#pragma omp parallel for private(j,k, i, tp1, tp2, cp1, c) shared(U) reduction(+:tmp4, tmp5, tmp6)
  for(i=0;i<Nx;i++){
    c = Int_Int_rho(U,i);
    cp1 = computeC_rho(U,i);
    tmp4 += dx*cp1 + c;   
    tmp5 += dx*Gridx((double)i)*cp1;
    tp1=0.; tp2=0.;
    for(j=0;j<size_v;j++){
      k = i*size_v + j;
      tp1 += (U[k*6+0] + U[k*6+5]/4.);
      tp2 += U[k*6+1];
    }
    tmp5 += scalev* (tp1*( (pow(Gridx(i+0.5), 3) - pow(Gridx(i-0.5), 3))/3. - Gridx(i-0.5)*Gridx((double)i)*dx ) - tp2 * dx*dx*Gridx((double)i)/12.);
    
    tp2 *= dx/2.;
    tmp6 +=  cp1*cp1*dx + 2*cp1*c + pow(dv, 6)* ( tp1*tp1*dx*dx*dx/3. + tp2*tp2*dx/30. - tp1*tp2*dx*dx/6.);//+ tp1*tp2*(dx*Gridx((double)i)/6. - dx*dx/4.) ); //Int_Cumulativerho_sqr(i);
  }
  retn = tmp1 + tmp2 + tmp3 + 2*ce1*tmp4 - 2*tmp5 + tmp6;
  return 0.5*retn;
}

double I1(double *U, int k, int l) // Calculate the first inegtral in H_(i,j), namely \int v1*f*phi_x dxdv
{
  double result;
  int i, j1, j2, j3; // k=i*Nv^3 + (j1*Nv*Nv + j2*Nv + j3)
  int j_mod = k%size_v;
  j3 = j_mod%Nv;
  j2 = ((j_mod-j3)%(Nv*Nv))/Nv;
  j1 = (j_mod-j3-j2*Nv)/(Nv*Nv);
  i = (k-j_mod)/size_v;

  if(l==1) result = dv*dv*dv*( Gridv((double)j1)*U[k*6+0] + dv*U[k*6+2]/12. + U[k*6+5]*Gridv((double)j1)/4.);
  else result=0.;
  
  return result;
}

double I2(double *U, int k, int l) // Calculate the fourth integral in H_(i,j), namely \int E*f*phi_v1 dxdv
{
  double result;
  int i, j1, j2, j3; // k=i*Nv^3 + (j1*Nv*Nv + j2*Nv + j3)
  int j = k%size_v;
  //j3 = j_mod%Nv;
  //j2 = ((j_mod-j3)%(Nv*Nv))/Nv;
  //j1 = (j_mod-j3-j2*Nv)/(Nv*Nv);
  i = (k-j)/size_v;

  if(l==2) result = Int_fE(U,i,j)/dv;
  else if(l==5) result = U[k*6+2]*dv*dv*intE[i]/6.; 
  else result = 0.;
  
  return result;
}

double I3(double *U, int k, int l) 																			// Calculate the difference of the second and third integrals in H_(i,j), namely \int_j v1*gh*phi dv at interface x=x_i+1/2 - \int_j v1*gh*phi dv at interface x=x_i-1/2
{
	double result, ur, ul;																					// declare result (the result of the integral to be returned), ur (used in the evaluation of gh^+/- on the right space cell edge) & ul (used in the evaluation of gh^+/- on the left space cell edge)
	int i, j1, j2, j3, iil, iir, kkl, kkr; 																	// declare i (the space cell coordinate), j1, j2, j3 (the coordinates of the velocity cell), iil (the cell from which the flux is flowing on the left of cell i in space), iir (the cell from which the flux is flowing on the right of cell i in space), kkl (the global index of the cell with coordinate (iil, j1, j2, j3)) & kkr (the global index of the cell with coordinate (iir, j1, j2, j3))
	int j_mod = k%size_v;																					// declare and calculate j_mod (the remainder when k is divided by size_v = Nv^3 - used to help determine the values of i, j1, j2 & j3 from the value of k)
	j3 = j_mod%Nv;																							// calculate j3 for the given k
	j2 = ((j_mod-j3)%(Nv*Nv))/Nv;																			// calculate j2 for the given k
	j1 = (j_mod-j3-j2*Nv)/(Nv*Nv);																			// calculate j1 for the given k
	i = (k-j_mod)/size_v;																					// calculate i for the given k

	if(j1<Nv/2)																								// do this if j1 < Nv/2 (so that the velocity in the v1 direction is negative)
	{
		iir=i+1; iil=i; 																					// set iir to the value of i+1 and iil to the value of i (as here the flow of information is from right to left so that gh^+ must be used at the cell edges)
		if(iir==Nx)iir=0; //periodic bc																		// if iir = Nx (the maximum value that can be obtained, since i = 0,1,...,Nx-1) and so this cell is at the right boundary, requiring information from the non-existent cell with space index Nx, since there are periodic boundary conditions, set iir = 0 and use the cell with space index 0 (i.e. the cell at the left boundary)
		kkr=iir*size_v + j_mod; 																			// calculate the value of kkr for this value of iir
		kkl=k;																								// set kkl to k (since iil = i)
		ur = -U[kkr*6+1]; 																					// set ur to the negative of the coefficient of the basis function with shape l which is non-zero in the cell with global index kkr (which corresponds to the evaluation of gh^+ at the right boundary and -ve since v_1 < 0 in here)
		ul = -U[kkl*6+1];																					// set ul to the negative of the coefficient of the basis function with shape l which is non-zero in the cell with global index kkl	(which corresponds to the evaluation of gh^+ at the left boundary and -ve since phi < 0 here)
	}
	else																									// do this if j1 >= Nv/2 (so that the velocity in the v1 direction is non-negative)
	{
		iir=i; iil=i-1;																						// set iir to the value of i and iil to the value of i-1 (as here the flow of information is from left to right so that gh^- must be used at the cell edges)
		if(iil==-1)iil=Nx-1; // periodic bc																	// if iil = -1 (the minimum value that can be obtained, since i = 0,1,...,Nx-1) and so this cell is at the left boundary, requiring information from the non-existent cell with space index -1, since there are periodic boundary conditions, set iil = Nx-1 and use the cell with space index Nx-1 (i.e. the cell at the right boundary)
		kkr=k; 																								// set kkr to k (since iir = i)
		kkl=iil*size_v + j_mod; 																			// calculate the value of kkl for this value of iil
		ur = U[kkr*6+1];																					// set ur to the value of the coefficient of the basis function with shape l which is non-zero in the cell with global index kkr (which corresponds to the evaluation of gh^- at the right boundary and +ve since v_1 >= 0 in here)
		ul = U[kkl*6+1];																					// set ul to the value of the coefficient of the basis function with shape l which is non-zero in the cell with global index kkl (which corresponds to the evalutaion of gh^- at the left boundary and +ve since v_r >= 0 in here)
	}
  
	if(l==0)result = dv*dv*dv*( (U[kkr*6+0]+0.5*ur - U[kkl*6+0]-0.5*ul)*Gridv((double)j1) + (U[kkr*6+2]-U[kkl*6+2])*dv/12. + (U[kkr*6+5]-U[kkl*6+5])*Gridv((double)j1)/4.);					// calculate \int_j v1*gh*phi dv at interface x=x_i+1/2 - \int_j v1*gh*phi dv at interface x=x_i-1/2 for the basis function with shape 0 (i.e. constant) which is non-zero in the cell with global index k
	if(l==1)result = 0.5*dv*dv*dv*( (U[kkr*6+0]+0.5*ur + U[kkl*6+0]+0.5*ul)*Gridv((double)j1) + (U[kkr*6+2]+U[kkl*6+2])*dv/12. + (U[kkr*6+5]+U[kkl*6+5])*Gridv((double)j1)/4.);				// calculate \int_j v1*gh*phi dv at interface x=x_i+1/2 - \int_j v1*gh*phi dv at interface x=x_i-1/2 for the basis function with shape 1 (i.e. linear in x) which is non-zero in the cell with global index k
	if(l==2)result = dv*dv*(( (U[kkr*6+0]-U[kkl*6+0])*dv*dv + (ur-ul)*0.5*dv*dv + (U[kkr*6+2]-U[kkl*6+2])*dv*Gridv((double)j1))/12. + (U[kkr*6+5]-U[kkl*6+5])*dv*dv*19./720.);				// calculate \int_j v1*gh*phi dv at interface x=x_i+1/2 - \int_j v1*gh*phi dv at interface x=x_i-1/2 for the basis function with shape 2 (i.e. linear in v_1) which is non-zero in the cell with global index k
	if(l==3)result = (U[kkr*6+3]-U[kkl*6+3])*Gridv((double)j1)*dv*dv*dv/12.;																												// calculate \int_j v1*gh*phi dv at interface x=x_i+1/2 - \int_j v1*gh*phi dv at interface x=x_i-1/2 for the basis function with shape 3 (i.e. linear in v_2) which is non-zero in the cell with global index k
	if(l==4)result = (U[kkr*6+4]-U[kkl*6+4])*Gridv((double)j1)*dv*dv*dv/12.;																												// calculate \int_j v1*gh*phi dv at interface x=x_i+1/2 - \int_j v1*gh*phi dv at interface x=x_i-1/2 for the basis function with shape 4 (i.e. linear in v_3) which is non-zero in the cell with global index k
	if(l==5)result = dv*dv*dv*((U[kkr*6+0] + 0.5*ur - U[kkl*6+0]-0.5*ul)*Gridv((double)j1)/4. + (U[kkr*6+2]-U[kkl*6+2])*dv*19./720. + (U[kkr*6+5]-U[kkl*6+5])*Gridv((double)j1)*19./240.);	// calculate \int_j v1*gh*phi dv at interface x=x_i+1/2 - \int_j v1*gh*phi dv at interface x=x_i-1/2 for the basis function with shape 5 (i.e. modulus of v) which is non-zero in the cell with global index k

	return result;
}

double I5(double *U, int k, int l) 	// Calculate the difference of the fifth and sixth integrals in H_(i,j), namely \int_i E*f*phi dx at interface v1==v_j+1/2 - \int_i E*f*phi dx at interface v1==v_j-1/2
{
	double result, ur, ul;																			// declare result (the result of the integral to be returned), ur (used in the evaluation of gh^+/- on the right velocity cell edge) & ul (used in the evaluation of gh^+/- on the left velocity cell edge)
	int i, j1, j2, j3, j1r, j1l, kkr, kkl; 															// declare i (the space cell coordinate), j1, j2, j3 (the coordinates of the velocity cell), j1l (the cell from which the flux is flowing on the left of the cell with coordinate j1 in the v1 direction), j1r (the cell from which the flux is flowing on the right of the cell with coordinate j1 in the v1 direction), kkl (the global index of the cell with coordinate (i, j1l, j2, j3)) & kkr (the global index of the cell with coordinate (i, j1r, j2, j3))
	int j_mod = k%size_v;																			// declare and calculate j_mod (the remainder when k is divided by size_v = Nv^3 - used to help determine the values of i, j1, j2 & j3 from the value of k)
	j3 = j_mod%Nv;																					// calculate j3 for the given k
	j2 = ((j_mod-j3)%(Nv*Nv))/Nv;																	// calculate j2 for the given k
	j1 = (j_mod-j3-j2*Nv)/(Nv*Nv);																	// calculate j1 for the given k
	i = (k-j_mod)/size_v;																			// calculate i for the given k

	//intE = Int_E(U,i); intE1 = Int_E1st(U,i); intE2 = Int_E2nd(U,i);
  
	if(intE[i]>0)																					// do this if the average direction of the field E over the space cell i is positive
	{
		j1r=j1+1;  j1l=j1;																			// set j1r to the value of j1+1 and j1l to the value of j1 (as here the the average flow of the field is from left to right so that gh^- must be used at the cell edges, as information flows against the field)
		kkr=i*size_v + (j1r*Nv*Nv + j2*Nv + j3);													// calculate the value of kkr for this value of j1r
		kkl=k; 																						// set kkl to k (since j1l = j1)
		if(j1r<Nv)ur = -U[kkr*6+2];																	// if j1r is not Nv (so that this cell is not receiving information from the right boundary), set ur to the negative of the coefficient of the basis function with shape 2 which is non-zero in the cell with global index kkr (which corresponds to the evaluation of gh^- at the right boundary and -ve since the field is negative here?) - note that if the cell was receiving information from the right boundary then gh^- = 0 here so ur is not needed
		ul = -U[kkl*6+2];																			// set ul to the negative of the coefficient of the basis function with shape 2 which is non-zero in the cell with global index kkl (which corresponds to the evaluation of gh^- at the right boundary and -ve since phi < 0 here)
	}
	else																							// do this if the average direction of the field E over the space cell i is non-positive
	{
		j1r=j1; j1l=j1-1;																			// set j1r to the value of j1 and j1l to the value of j1-1 (as here the the average flow of the field is from right to left so that gh^+ must be used at the cell edges, as information flows against the field)
		kkr=k;																						// set kkr to k (since j1r = j1)
		kkl=i*size_v + (j1l*Nv*Nv + j2*Nv + j3);													// calculate the value of kkl for this value of j1l
		ur = U[kkr*6+2];																			// set ur to the the coefficient of the basis function with shape 2 which is non-zero in the cell with global index kkr (which corresponds to the evaluation of gh^+ at the left boundary and +ve since phi > 0 here)
		if(j1l>-1)ul = U[kkl*6+2];																	// if j1l is not -1 (so that this cell is not receiving information from the left boundary), set ul to the coefficient of the basis function with shape 2 which is non-zero in the cell with global index kkl (which corresponds to the evaluation of gh^+ at the left boundary and +ve since phi < 0 here and being subtracted?) - note that if the cell was receiving information from the left boundary then gh^+ = 0 here so ul is not needed
	}

	if(l==0)																						// calculate \int_i E*f*phi dx at interface v1==v_j+1/2 - \int_i E*f*phi dx at interface v1==v_j-1/2 for the basis function with shape 0 (i.e. constant) which is non-zero in the cell with global index k
	{
		if(j1r<Nv && j1l>-1) result = dv*dv*(U[kkr*6+0] + 0.5*ur + U[kkr*6+5]*5./12.- U[kkl*6+0] - 0.5*ul - U[kkl*6+5]*5./12.)*intE[i] + dv*dv*(U[kkr*6+1]-U[kkl*6+1])*intE1[i];		// this is the value at an interior cell
		else if(j1r<Nv)result =   dv*dv*(U[kkr*6+0] + 0.5*ur + U[kkr*6+5]*5./12.)*intE[i] + dv*dv*U[kkr*6+1]*intE1[i];																	// this is the value at the cell at the left boundary in v1 (so that the integral over the left edge is zero)
		else if(j1l>-1)result = dv*dv*(- U[kkl*6+0] - 0.5*ul - U[kkl*6+5]*5./12.)*intE[i] - dv*dv*U[kkl*6+1]*intE1[i];																	// this is the value at the cell at the right boundary in v1 (so that the integral over the right edge is zero)
	}
	if(l==1)																						// calculate \int_i E*f*phi dx at interface v1==v_j+1/2 - \int_i E*f*phi dx at interface v1==v_j-1/2 for the basis function with shape 1 (i.e. linear in x) which is non-zero in the cell with global index k
	{
		if(j1r<Nv && j1l>-1) result = dv*dv*( (U[kkr*6+0] + 0.5*ur + U[kkr*6+5]*5./12. - U[kkl*6+0] - 0.5*ul - U[kkl*6+5]*5./12.)*intE1[i] + (U[kkr*6+1] - U[kkl*6+1])*intE2[i] );		// this is the value at an interior cell
		else if(j1r<Nv)result=dv*dv*( (U[kkr*6+0] + 0.5*ur + U[kkr*6+5]*5./12.)*intE1[i] + U[kkr*6+1]*intE2[i] );																		// this is the value at the cell at the left boundary in v1 (so that the integral over the left edge is zero)
		else if(j1l>-1)result=dv*dv*( (- U[kkl*6+0] - 0.5*ul - U[kkl*6+5]*5./12.)*intE1[i] - U[kkl*6+1]*intE2[i] );																		// this is the value at the cell at the right boundary in v1 (so that the integral over the right edge is zero)
	}
	if(l==2)																						// calculate \int_i E*f*phi dx at interface v1==v_j+1/2 - \int_i E*f*phi dx at interface v1==v_j-1/2 for the basis function with shape 2 (i.e. linear in v1) which is non-zero in the cell with global index k
	{
		if(j1r<Nv && j1l>-1)result = 0.5*(dv*dv*(U[kkr*6+0] + 0.5*ur + U[kkr*6+5]*5./12.+ U[kkl*6+0] + 0.5*ul + U[kkl*6+5]*5./12.)*intE[i] + dv*dv*(U[kkr*6+1]+U[kkl*6+1])*intE1[i]);	// this is the value at an interior cell
		else if(j1r<Nv)result = 0.5*(dv*dv*(U[kkr*6+0] + 0.5*ur + U[kkr*6+5]*5./12.)*intE[i] + dv*dv*U[kkr*6+1]*intE1[i]);																// this is the value at the cell at the left boundary in v1 (so that the integral over the left edge is zero)
		else if(j1l>-1)result = 0.5*(dv*dv*(U[kkl*6+0] + 0.5*ul + U[kkl*6+5]*5./12.)*intE[i] + dv*dv*U[kkl*6+1]*intE1[i]);    															// this is the value at the cell at the right boundary in v1 (so that the integral over the right edge is zero)
	}
	if(l==3)																						// calculate \int_i E*f*phi dx at interface v1==v_j+1/2 - \int_i E*f*phi dx at interface v1==v_j-1/2 for the basis function with shape 3 (i.e. linear in v2) which is non-zero in the cell with global index k
	{
		if(j1r<Nv && j1l>-1)result = (U[kkr*6+3]-U[kkl*6+3])*intE[i]*dv*dv/12.;						// this is the value at an interior cell
		else if(j1r<Nv)result = U[kkr*6+3]*intE[i]*dv*dv/12.;										// this is the value at the cell at the left boundary in v1 (so that the integral over the left edge is zero)
		else if(j1l>-1)result = -U[kkl*6+3]*intE[i]*dv*dv/12.;										// this is the value at the cell at the right boundary in v1 (so that the integral over the right edge is zero)
	}
	if(l==4)																						// calculate \int_i E*f*phi dx at interface v1==v_j+1/2 - \int_i E*f*phi dx at interface v1==v_j-1/2 for the basis function with shape 4 (i.e. linear in v3) which is non-zero in the cell with global index k
	{
		if(j1r<Nv && j1l>-1)result = (U[kkr*6+4]-U[kkl*6+4])*intE[i]*dv*dv/12.;						// this is the value at an interior cell
		else if(j1r<Nv)result=U[kkr*6+4]*intE[i]*dv*dv/12.;											// this is the value at the cell at the left boundary in v1 (so that the integral over the left edge is zero)
		else if(j1l>-1)result=-U[kkl*6+4]*intE[i]*dv*dv/12.;										// this is the value at the cell at the right boundary in v1 (so that the integral over the right edge is zero)
	}
	if(l==5)																						// calculate \int_i E*f*phi dx at interface v1==v_j+1/2 - \int_i E*f*phi dx at interface v1==v_j-1/2 for the basis function with shape 5 (i.e. the modulus of v) which is non-zero in the cell with global index k
	{
		if(j1r<Nv && j1l>-1)result = dv*dv*( ((U[kkr*6+0] + 0.5*ur - U[kkl*6+0] - 0.5*ul)*5./12. + (U[kkr*6+5]- U[kkl*6+5])*133./720.)*intE[i] + (U[kkr*6+1] - U[kkl*6+1])*intE1[i]*5./12. ); //BUG: coefficient of U[k][5] was 11/48 insteadof 133/720		// this is the value at an interior cell
		else if(j1r<Nv)result= dv*dv*( ((U[kkr*6+0] + 0.5*ur)*5./12. + U[kkr*6+5]*133./720.)*intE[i] + U[kkr*6+1]*intE1[i]*5./12. );													// this is the value at the cell at the left boundary in v1 (so that the integral over the left edge is zero)
		else if(j1l>-1)result=-dv*dv*( ((U[kkl*6+0] + 0.5*ul)*5./12. + U[kkl*6+5]*133./720.)*intE[i] + U[kkl*6+1]*intE1[i]*5./12. );													// this is the value at the cell at the right boundary in v1 (so that the integral over the right edge is zero)
	}

  	return result;
}

#ifdef MPI
void computeH(double *H, double *U)// H_k(i,j)(f, E, phi_l)  
{
  int k, l; // k=i*Nv^3 + (j1*Nv*Nv + j2*Nv + j3)
  double tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tp0, tp5;
 
  #pragma omp parallel for schedule(dynamic) private(tp0, tp5, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6) shared(H, U)
  for(k=chunksize_dg*myrank_mpi;k<chunksize_dg*(myrank_mpi+1);k++){        
	tp0 = I1(U,k,0) - I2(U,k,0) - I3(U,k,0) + I5(U,k,0);
	tp5 = I1(U,k,5) - I2(U,k,5) - I3(U,k,5) + I5(U,k,5);
	H[(k%chunksize_dg)*6+0] = (19*tp0/4. - 15*tp5)/(dx*dv*dv*dv);
	H[(k%chunksize_dg)*6+5] = (60*tp5 - 15*tp0)/(dx*dv*dv*dv);	
    for(l=1;l<5;l++){
	  tmp1=I1(U,k,l); tmp2=I2(U,k,l); tmp3=I3(U,k,l);  tmp5=I5(U,k,l); 
	  H[(k%chunksize_dg)*6+l] = (tmp1-tmp2-tmp3+tmp5)*12./(dx*dv*dv*dv);				  
	}	
  }  
}

void RK3(double *U) // RK3 for f_t = H(f)
{
  int i, k, l, k_local;
  double tp0, tp1, tp2, tp3, tp4, tp5, H[6];//, tp0, tp5, tmp1, tmp2, tmp3, tmp5;
 
  MPI_Status status;
  
  ce = computePhi_x_0(U); 

  #pragma omp parallel for private(i) shared(U,cp, intE, intE1)
  for(i=0;i<Nx;i++){
    cp[i] = computeC_rho(U,i); intE[i] = Int_E(U,i); intE1[i] = Int_E1st(U,i); 
  }
  
  #pragma omp parallel for private(i) shared(intE2)
  for(i=0;i<Nx;i++){
    intE2[i] = Int_E2nd(U,i); // BUG: Int_E2nd() require knowldege of cp 
  }
  
  #pragma omp parallel for schedule(dynamic)  private(H,k, k_local, l, tp0, tp1, tp2, tp3, tp4, tp5) shared(U, Utmp)
  for(k=chunksize_dg*myrank_mpi;k<chunksize_dg*(myrank_mpi+1);k++){ 
    k_local = k%chunksize_dg;
    
    tp0=I1(U,k,0)-I2(U,k,0)-I3(U,k,0)+I5(U,k,0);
    tp1=I1(U,k,1)-I2(U,k,1)-I3(U,k,1)+I5(U,k,1);
    tp2=I1(U,k,2)-I2(U,k,2)-I3(U,k,2)+I5(U,k,2);
    tp3=I1(U,k,3)-I2(U,k,3)-I3(U,k,3)+I5(U,k,3);
    tp4=I1(U,k,4)-I2(U,k,4)-I3(U,k,4)+I5(U,k,4);
    tp5=I1(U,k,5)-I2(U,k,5)-I3(U,k,5)+I5(U,k,5);

    //H[k_local][0] = (19*tp[0]/4. - 15*tp[5])/dx/scalev;
    //H[k_local][5] = (60*tp[5] - 15*tp[0])/dx/scalev;	
    H[0] = (19*tp0/4. - 15*tp5)/dx/scalev;
    H[5] = (60*tp5 - 15*tp0)/dx/scalev;	
    //for(l=1;l<5;l++)H[l] = tp[l]*12./dx/scalev;;//H[k_local][l] = tp[l]*12./dx/scalev;
    H[1] = tp1*12./dx/scalev; H[2] = tp2*12./dx/scalev; H[3] = tp3*12./dx/scalev; H[4] = tp4*12./dx/scalev;
    
    for(l=0;l<6;l++) Utmp[k_local*6+l] = U[k*6+l] + dt*H[l];	
  }    
  if(myrank_mpi == 0) {
    //dump the weights we've computed into U1
    for(k=0;k<chunksize_dg;k++) {
	for(l=0;l<6;l++) U1[k*6+l] = Utmp[k*6+l];	
    } 
    //receive from all other processes
    for(i=1;i<nprocs_mpi;i++) {
	    MPI_Recv(output_buffer_vp, chunksize_dg*6, MPI_DOUBLE, i, i, MPI_COMM_WORLD, &status); // receive weight from other processes consecutively, rank i=1..numNodes-1, ensuring the weights are stored in the file consecutively !
	    for(k=0;k<chunksize_dg;k++) {
			for(l=0;l<6;l++)U1[(k + i*chunksize_dg)*6+l] = output_buffer_vp[k*6+l];
		}			
    }
  }
  else  MPI_Send(Utmp, chunksize_dg*6, MPI_DOUBLE, 0, myrank_mpi, MPI_COMM_WORLD);
  
  
  MPI_Bcast(U1, size*6, MPI_DOUBLE, 0, MPI_COMM_WORLD);    
  MPI_Barrier(MPI_COMM_WORLD);
  /////////////////// 1st step of RK3 done//////////////////////////////////////////////////////// 
    
  ce = computePhi_x_0(U1); 

  #pragma omp parallel for private(i) shared(U1,cp, intE, intE1)
  for(i=0;i<Nx;i++){
    cp[i] = computeC_rho(U1,i); intE[i] = Int_E(U1,i); intE1[i] = Int_E1st(U1,i); 
  }
  
  #pragma omp parallel for private(i) shared(U1,intE2)
  for(i=0;i<Nx;i++){
    intE2[i] = Int_E2nd(U1,i); // BUG: Int_E2nd() require knowldege of cp 
  }
  
  #pragma omp parallel for schedule(dynamic) private(H, k, k_local, l, tp0, tp1, tp2, tp3, tp4, tp5)  shared(U,Utmp)
  for(k=chunksize_dg*myrank_mpi;k<chunksize_dg*(myrank_mpi+1);k++){      
    k_local = k%chunksize_dg;
    
    tp0=I1(U1,k,0)-I2(U1,k,0)-I3(U1,k,0)+I5(U1,k,0);
    tp1=I1(U1,k,1)-I2(U1,k,1)-I3(U1,k,1)+I5(U1,k,1);
    tp2=I1(U1,k,2)-I2(U1,k,2)-I3(U1,k,2)+I5(U1,k,2);
    tp3=I1(U1,k,3)-I2(U1,k,3)-I3(U1,k,3)+I5(U1,k,3);
    tp4=I1(U1,k,4)-I2(U1,k,4)-I3(U1,k,4)+I5(U1,k,4);
    tp5=I1(U1,k,5)-I2(U1,k,5)-I3(U1,k,5)+I5(U1,k,5);

    //H[k_local][0] = (19*tp[0]/4. - 15*tp[5])/dx/scalev;
    //H[k_local][5] = (60*tp[5] - 15*tp[0])/dx/scalev;	
    H[0] = (19*tp0/4. - 15*tp5)/dx/scalev;
    H[5] = (60*tp5 - 15*tp0)/dx/scalev;	
    //for(l=1;l<5;l++)H[l] = tp[l]*12./dx/scalev;;//H[k_local][l] = tp[l]*12./dx/scalev;
    H[1] = tp1*12./dx/scalev; H[2] = tp2*12./dx/scalev; H[3] = tp3*12./dx/scalev; H[4] = tp4*12./dx/scalev;
    
    for(l=0;l<6;l++) Utmp[k_local*6+l] = 0.75*U[k*6+l] + 0.25*U1[k*6+l] + 0.25*dt*H[l];
  }    
  if(myrank_mpi == 0) {
    //dump the weights we've computed into U1
    for(k=0;k<chunksize_dg;k++) {
	      for(l=0;l<6;l++) U1[k*6+l] = Utmp[k*6+l];	
    } 
    //receive from all other processes
    for(i=1;i<nprocs_mpi;i++) {      
		MPI_Recv(output_buffer_vp, chunksize_dg*6, MPI_DOUBLE, i, i, MPI_COMM_WORLD, &status); // receive weight from other processes consecutively, rank i=1..numNodes-1, ensuring the weights are stored in the file consecutively !
		for(k=0;k<chunksize_dg;k++) {
			for(l=0;l<6;l++)U1[(k + i*chunksize_dg)*6+l] = output_buffer_vp[k*6+l];
        }
    }
  }
  else MPI_Send(Utmp, chunksize_dg*6, MPI_DOUBLE, 0, myrank_mpi, MPI_COMM_WORLD);
  
  
  MPI_Bcast(U1, size*+6, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
  /////////////////// 2nd step of RK3 done//////////////////////////////////////////////////////// 
   
ce = computePhi_x_0(U1); 

  #pragma omp parallel for private(i) shared(U1,cp, intE, intE1)
  for(i=0;i<Nx;i++){
    cp[i] = computeC_rho(U1,i); intE[i] = Int_E(U1,i); intE1[i] = Int_E1st(U1,i); 
  }
  
  #pragma omp parallel for private(i) shared(U1,intE2)
  for(i=0;i<Nx;i++){
    intE2[i] = Int_E2nd(U1,i); // BUG: Int_E2nd() require knowldege of cp 
  }
  
  #pragma omp parallel for schedule(dynamic) private(H, k, k_local, l, tp0, tp1, tp2, tp3, tp4, tp5)  shared(U,Utmp)
  for(k=chunksize_dg*myrank_mpi;k<chunksize_dg*(myrank_mpi+1);k++){      
    k_local = k%chunksize_dg;
  
    tp0=I1(U1,k,0)-I2(U1,k,0)-I3(U1,k,0)+I5(U1,k,0);
    tp1=I1(U1,k,1)-I2(U1,k,1)-I3(U1,k,1)+I5(U1,k,1);
    tp2=I1(U1,k,2)-I2(U1,k,2)-I3(U1,k,2)+I5(U1,k,2);
    tp3=I1(U1,k,3)-I2(U1,k,3)-I3(U1,k,3)+I5(U1,k,3);
    tp4=I1(U1,k,4)-I2(U1,k,4)-I3(U1,k,4)+I5(U1,k,4);
    tp5=I1(U1,k,5)-I2(U1,k,5)-I3(U1,k,5)+I5(U1,k,5);

    H[0] = (19*tp0/4. - 15*tp5)/dx/scalev;
    H[5] = (60*tp5 - 15*tp0)/dx/scalev;	
    //for(l=1;l<5;l++)H[l] = tp[l]*12./dx/scalev;;//H[k_local][l] = tp[l]*12./dx/scalev;
    H[1] = tp1*12./dx/scalev; H[2] = tp2*12./dx/scalev; H[3] = tp3*12./dx/scalev; H[4] = tp4*12./dx/scalev;	

    for(l=0;l<6;l++) Utmp[k_local*6+l] = U[k*6+l]/3. + U1[k*6+l]*2./3. + dt*H[l]*2./3.;
  }    
  if(myrank_mpi == 0) {
    //dump the weights we've computed into U1
    for(k=0;k<chunksize_dg;k++) {
	      for(l=0;l<6;l++) U[k*6+l] = Utmp[k*6+l];	
    } 
    //receive from all other processes
    for(i=1;i<nprocs_mpi;i++) {      
	    MPI_Recv(output_buffer_vp, chunksize_dg*6, MPI_DOUBLE, i, i, MPI_COMM_WORLD, &status); // receive weight from other processes consecutively, rank i=1..numNodes-1, ensuring the weights are stored in the file consecutively !
	    for(k=0;k<chunksize_dg;k++) {
		  for(l=0;l<6;l++)U[(k + i*chunksize_dg)*6+l] = output_buffer_vp[k*6+l];
        } 
    }
  }
  else MPI_Send(Utmp, chunksize_dg*6, MPI_DOUBLE, 0, myrank_mpi, MPI_COMM_WORLD);
  
  MPI_Bcast(U, size*6, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD); 
  /////////////////// 3rd step of RK3 done//////////////////////////////////////////////////////// 
}


#else
void computeH(double *U)// H_k(i,j)(f, E, phi_l)  
{
  int i, k, l; // k=i*Nv^3 + (j1*Nv*Nv + j2*Nv + j3)
  double tp0, tp1, tp2, tp3, tp4, tp5;
 

  ce = computePhi_x_0(U); 
  // #pragma omp barrier
  #pragma omp parallel for private(i) shared(U,cp, intE, intE1)
  for(i=0;i<Nx;i++){
    cp[i] = computeC_rho(U,i); intE[i] = Int_E(U,i); intE1[i] = Int_E1st(U,i); 
  }
  
  #pragma omp parallel for private(i) shared(U, intE2)
  for(i=0;i<Nx;i++){
    intE2[i] = Int_E2nd(U,i); // BUG: Int_E2nd() require knowldege of cp 
  }
  
  #pragma omp parallel for schedule(dynamic) private(tp0, tp1, tp2, tp3, tp4, tp5, k,l) shared(U, H)
  for(k=0;k<size;k++){
   // double tp[6];

  // #pragma omp critical
   // {
      /*for(l=0;l<6;l++){
	tp[l]=I1(U,k,l)-I2(U,k,l)-I3(U,k,l)+I5(U,k,l);		
      } */
   // }
    tp0=I1(U,k,0)-I2(U,k,0)-I3(U,k,0)+I5(U,k,0);
    tp1=I1(U,k,1)-I2(U,k,1)-I3(U,k,1)+I5(U,k,1);
    tp2=I1(U,k,2)-I2(U,k,2)-I3(U,k,2)+I5(U,k,2);
    tp3=I1(U,k,3)-I2(U,k,3)-I3(U,k,3)+I5(U,k,3);
    tp4=I1(U,k,4)-I2(U,k,4)-I3(U,k,4)+I5(U,k,4);
    tp5=I1(U,k,5)-I2(U,k,5)-I3(U,k,5)+I5(U,k,5);
     
    H[k*6+0] = (19*tp0/4. - 15*tp5)/dx/scalev;
    H[k*6+5] = (60*tp5 - 15*tp0)/dx/scalev;	
    //for(l=1;l<5;l++)H[k][l] = tp[l]*12./dx/scalev;

    H[k*6+1] = tp1*12./dx/scalev; H[k*6+2] = tp2*12./dx/scalev; H[k*6+3] = tp3*12./dx/scalev; H[k*6+4] = tp4*12./dx/scalev;
  }
  
}

void RK3(double *U) // RK3 for f_t = H(f)
{
  int k, l;
  //double **H = (double **)malloc(size*sizeof(double *));
  //for (l=0;l<size;l++) H[l] = (double*)malloc(6*sizeof(double));
  //double **U1 = (double **)malloc(size*sizeof(double *));
  //for (l=0;l<size;l++) U1[l] = (double*)malloc(6*sizeof(double));
  
  computeH(U);
   #pragma omp parallel for private(k,l) shared(U)
  for(k=0;k<size;k++){				  
	  for(l=0;l<6;l++) U1[k*6+l] = U[k*6+l] + dt*H[k*6+l];				  
  }
 
  computeH(U1);
   #pragma omp parallel for private(k,l) shared(U)
  for(k=0;k<size;k++){						  
	  for(l=0;l<6;l++) U1[k*6+l] = 0.75*U[k*6+l] + 0.25*U1[k*6+l] + 0.25*dt*H[k*6+l];		 
  }

  computeH(U1);
   #pragma omp parallel for private(k,l) shared(U)
  for(k=0;k<size;k++){						  
	for(l=0;l<6;l++) U[k*6+l] = U[k*6+l]/3. + U1[k*6+l]*2./3. + dt*H[k*6+l]*2./3.;			
  }
  //free(H); free(U1);
}
#endif
