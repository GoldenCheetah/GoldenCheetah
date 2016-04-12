## R script to install packages for embedded R
##
## to run this script: 
## $ R CMD BATCH install-packages.R
## $ cat install-packages.Rout
##
## personal libs
dir.create(Sys.getenv("R_LIBS_USER"), showWarnings = FALSE, recursive = TRUE)

##We don't install a standard package anymore, since we need to build it
##with callbacks enabled
##install.packages("RInside", Sys.getenv("R_LIBS_USER"), repos = "http://cran.case.edu" )
install.packages("Rcpp", Sys.getenv("R_LIBS_USER"), repos = "http://cran.case.edu" )
