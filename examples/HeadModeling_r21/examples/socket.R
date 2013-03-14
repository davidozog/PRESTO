vis_histogram <- function(host = "localhost") {

    a <- make.socket(host, 11110)
    on.exit(close.socket(a))

    write.socket(a, "alive")


    while (TRUE) {
      output <- character(0)

      ss <- read.socket(a)
      if (ss == "") break
      output <- paste(output, ss)

      cat(output)
      invisible(output)

      write.socket(a, "vis_hist here")

    x=rnorm(1000,mean=0,sd=1)
    x
    hist(x)

    Sys.sleep(1)


    }

    close.socket(a)

}
vis_histogram("localhost")
