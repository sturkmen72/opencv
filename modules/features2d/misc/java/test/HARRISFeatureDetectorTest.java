package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;
import org.opencv.test.OpenCVTestRunner;
import org.opencv.xfeatures2d.HarrisLaplaceFeatureDetector;

public class HARRISFeatureDetectorTest extends OpenCVTestCase {

    HarrisLaplaceFeatureDetector detector;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        detector = HarrisLaplaceFeatureDetector.create(); // default constructor have (6, 0.01, 0.01, 5000, 4)
    }

    public void testCreate() {
        assertNotNull(detector);
    }

    public void testDetectListOfMatListOfListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectListOfMatListOfListOfKeyPointListOfMat() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPointMat() {
        fail("Not yet implemented");
    }

    public void testEmpty() {
        fail("Not yet implemented");
    }

    public void testRead() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        writeFile(filename, "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.HARRIS-LAPLACE</name>\n<numOctaves>5</numOctaves>\n<corn_thresh>0.02</corn_thresh>\n<DOG_thresh>0.03</DOG_thresh>\n<maxCorners>4000</maxCorners>\n<num_layers>2</num_layers>\n</opencv_storage>\n");
        detector.read(filename);

        assertEquals(5, detector.getNumOctaves());
        assertEquals((float) 0.02, detector.getCornThresh());
        assertEquals((float) 0.03, detector.getDOGThresh());
        assertEquals(4000, detector.getMaxCorners());
        assertEquals(2, detector.getNumLayers());
    }

    public void testReadYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        writeFile(filename, "%YAML:1.0\n---\nname: \"Feature2D.HARRIS-LAPLACE\"\nnumOctaves: 5\ncorn_thresh: 0.02\nDOG_thresh: 0.03\nmaxCorners: 4000\nnum_layers: 2\n");
        detector.read(filename);

        assertEquals(5, detector.getNumOctaves());
        assertEquals((float) 0.02, detector.getCornThresh());
        assertEquals((float) 0.03, detector.getDOGThresh());
        assertEquals(4000, detector.getMaxCorners());
        assertEquals(2, detector.getNumLayers());
    }

    public void testWrite() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        detector.write(filename);

        String truth = "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.HARRIS-LAPLACE</name>\n<numOctaves>6</numOctaves>\n<corn_thresh>9.9999997764825821e-03</corn_thresh>\n<DOG_thresh>9.9999997764825821e-03</DOG_thresh>\n<maxCorners>5000</maxCorners>\n<num_layers>4</num_layers>\n</opencv_storage>\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        detector.write(filename);

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.HARRIS-LAPLACE\"\nnumOctaves: 6\ncorn_thresh: 9.9999997764825821e-03\nDOG_thresh: 9.9999997764825821e-03\nmaxCorners: 5000\nnum_layers: 4\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

}
